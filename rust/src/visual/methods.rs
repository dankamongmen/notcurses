//! `NcVisual*` methods and associated functions.

use core::ptr::null_mut;
use libc::c_void;

use crate::{
    cstring, error, error_ref_mut, rstring, NcBlitter, NcDim, NcDirect, NcDirectF, NcDirectV,
    NcError, NcIntResult, NcPixel, NcPlane, NcResult, NcRgba, NcScale, NcTime, NcVGeom, NcVisual,
    NcVisualOptions, Notcurses, NCBLIT_PIXEL, NCRESULT_ERR,
};

/// # NcVisualOptions Constructors
impl NcVisualOptions {
    // /// New NcVisualOptions
    // pub fn new() -> Self {
    //     Self::with_flags()
    // }
    //
    // pub fn new_aligned() -> Self {
    //     Self::with_flags_aligned()
    // }

    // TODO:
    // - horizontally aligned
    // - copy from NcPlaneOptions (with_flags_aligned & with_flags,)
    // y is an ncalign_e if NCVISUAL_OPTION_VERALIGNED is provided.
    // x is an ncalign_e value if NCVISUAL_OPTION_HORALIGNED is provided.

    /// Specify an existing plane.
    ///
    /// If [`NCVISUAL_OPTION_CHILDPLANE`][crate::NCVISUAL_OPTION_CHILDPLANE] is
    /// used in `flags` then the `plane` is interpreted as the parent [`NcPlane`]
    /// of the new plane created for this [`NcVisual`].
    pub fn with_plane(
        plane: &mut NcPlane,
        scale: NcScale,
        y: NcDim,
        x: NcDim,
        begy: NcDim,
        begx: NcDim,
        leny: NcDim,
        lenx: NcDim,
        blitter: NcBlitter,
        flags: u32,
        transcolor: NcRgba,
    ) -> Self {
        Self {
            // provided plane
            n: plane,
            // the source is stretched/scaled relative to the provided ncplane
            scaling: scale,
            y: y as i32,
            x: x as i32,
            // origin of rendered section
            begy: begy as i32,
            begx: begx as i32,
            // size of rendered section
            leny: leny as i32,
            lenx: lenx as i32,
            // glyph set to use
            blitter,
            // bitmask over NCVISUAL_OPTION_*
            flags: flags as u64,
            transcolor,
        }
    }

    pub fn without_plane(
        y: NcDim,
        x: NcDim,
        begy: NcDim,
        begx: NcDim,
        leny: NcDim,
        lenx: NcDim,
        blitter: NcBlitter,
        flags: u32,
        transcolor: u32,
    ) -> Self {
        Self {
            n: null_mut(),
            scaling: crate::NCSCALE_NONE,
            // where the created ncplane will be placed relative to the standard plane's origin
            y: y as i32,
            x: x as i32,
            // origin of rendered section
            begy: begy as i32,
            begx: begx as i32,
            // size of rendered section
            leny: leny as i32,
            lenx: lenx as i32,
            // glyph set to use
            blitter,
            // bitmask over NCVISUAL_OPTION_*
            flags: flags as u64,
            // This color will be treated as transparent with flag [NCVISUAL_OPTION_ADDALPHA].
            transcolor,
        }
    }

    pub fn fullsize_pixel_without_plane(y: NcDim, x: NcDim, leny: NcDim, lenx: NcDim) -> Self {
        Self::without_plane(y, x, 0, 0, leny, lenx, NCBLIT_PIXEL, 0, 0)
    }
}

/// # NcVisual Constructors & destructors
impl NcVisual {
    /// Like [from_rgba][NcVisual#method.from_rgba], but 'bgra' is arranged as BGRA.
    ///
    /// *C style function: [ncvisual_from_bgra()][crate::ncvisual_from_bgra].*
    pub fn from_bgra<'a>(
        bgra: &[u8],
        rows: NcDim,
        rowstride: NcDim,
        cols: NcDim,
    ) -> NcResult<&'a mut NcVisual> {
        error_ref_mut![
            unsafe {
                crate::ncvisual_from_bgra(
                    bgra.as_ptr() as *const c_void,
                    rows as i32,
                    rowstride as i32,
                    cols as i32,
                )
            },
            &format![
                "NcVisual::from_bgra(bgra, {}, {}, {})",
                rows, rowstride, cols
            ]
        ]
    }

    /// Opens a visual at `file`, extracts the codec and parameters and
    /// decodes the first image to memory.
    ///
    /// *C style function: [ncvisual_from_file()][crate::ncvisual_from_file].*
    pub fn from_file<'a>(file: &str) -> NcResult<&'a mut NcVisual> {
        error_ref_mut![
            unsafe { crate::ncvisual_from_file(cstring![file]) },
            &format!("NcVisual::from_file({})", file)
        ]
    }

    /// Promotes an NcPlane to an NcVisual.
    ///
    /// The plane may contain only spaces, half blocks, and full blocks.
    /// This will be checked, and any other glyph will result in an error.
    ///
    /// This function exists so that planes can be subjected to NcVisual transformations.
    /// If possible, it's better to create the ncvisual from memory using
    /// [from_rgba][NcVisual#method.from_rgba].
    ///
    /// *C style function: [ncvisual_from_plane()][crate::ncvisual_from_plane].*
    pub fn from_plane<'a>(
        plane: &NcPlane,
        blitter: NcBlitter,
        beg_y: NcDim,
        beg_x: NcDim,
        len_y: NcDim,
        len_x: NcDim,
    ) -> NcResult<&'a mut NcVisual> {
        error_ref_mut![
            unsafe {
                crate::ncvisual_from_plane(
                    plane,
                    blitter,
                    beg_y as i32,
                    beg_x as i32,
                    len_y as i32,
                    len_x as i32,
                )
            },
            &format!(
                "NcVisual::from_file(plane, {}, {}, {}, {}, {})",
                blitter, beg_y, beg_x, len_y, len_x
            )
        ]
    }

    /// Prepares an NcVisual, and its underlying NcPlane, based off RGBA content
    /// in memory at `rgba`.
    ///
    /// `rgba` is laid out as `rows` lines, each of which is `rowstride` bytes in length.
    /// Each line has `cols` 32-bit 8bpc RGBA pixels followed by possible padding
    /// (there will be rowstride - cols * 4 bytes of padding).
    ///
    /// The total size of `rgba` is thus (rows * rowstride) bytes, of which
    /// (rows * cols * 4) bytes are actual non-padding data.
    ///
    /// *C style function: [ncvisual_from_rgba()][crate::ncvisual_from_rgba].*
    pub fn from_rgba<'a>(
        rgba: &[u8],
        rows: NcDim,
        rowstride: NcDim,
        cols: NcDim,
    ) -> NcResult<&'a mut NcVisual> {
        error_ref_mut![
            unsafe {
                crate::ncvisual_from_rgba(
                    rgba.as_ptr() as *const c_void,
                    rows as i32,
                    rowstride as i32,
                    cols as i32,
                )
            },
            &format!(
                "NcVisual::from_rgba(rgba, {}, {}, {})",
                rows, rowstride, cols
            )
        ]
    }

    /// Destroys this NcVisual.
    ///
    /// Rendered elements will not be disrupted, but the visual can be neither
    /// decoded nor rendered any further.
    ///
    /// *C style function: [ncvisual_destroy()][crate::ncvisual_destroy].*
    pub fn destroy(&mut self) {
        unsafe { crate::ncvisual_destroy(self) }
    }
}

/// # NcVisual Methods
impl NcVisual {
    /// Gets the specified pixel from this NcVisual.
    ///
    /// *C style function: [ncvisual_at_yx()][crate::ncvisual_at_yx].*
    pub fn at_yx(&self, y: NcDim, x: NcDim) -> NcResult<NcPixel> {
        let mut pixel = 0;
        let res = unsafe { crate::ncvisual_at_yx(self, y as i32, x as i32, &mut pixel) };
        error![res, "NcVisual.at_yx()", pixel]
    }

    /// Extracts the next frame from the NcVisual.
    ///
    /// Returns 0 for normal frames, and 1 to indicate EOF.
    ///
    /// *C style function: [ncvisual_decode()][crate::ncvisual_decode].*
    pub fn decode(&mut self) -> NcResult<NcIntResult> {
        let res = unsafe { crate::ncvisual_decode(self) };
        if res == NCRESULT_ERR {
            Err(NcError::with_msg(res, "NcVisual.decode()"))
        } else {
            Ok(res)
        }
    }

    /// Extracts the next frame from the NcVisual, ala [decode][NcVisual#method.decode],
    /// but if we have reached the end, rewinds to the first frame.
    ///
    /// *A subsequent [NcVisual.render]() will render the first frame,
    /// as if the ncvisual had been closed and reopened.*
    ///
    /// Returns 0 for normal frames and 1 to indicate EOF.
    ///
    /// *C style function: [ncvisual_decode_loop()][crate::ncvisual_decode_loop].*
    pub fn decode_loop(&mut self) -> NcResult<NcIntResult> {
        let res = unsafe { crate::ncvisual_decode_loop(self) };
        if res == NCRESULT_ERR {
            Err(NcError::with_msg(res, "NcVisual.decode_loop()"))
        } else {
            Ok(res)
        }
    }

    /// Inflates each pixel in the image to 'scale'x'scale' pixels.
    ///
    /// The original color is retained.
    pub fn inflate(&mut self, scale: u32) -> NcResult<NcIntResult> {
        let res = unsafe { crate::ncvisual_inflate(self, scale as i32) };
        error![res, &format!["NcVisual.inflate({})", scale], res]
    }

    /// Gets the size and ratio of NcVisual pixels to output cells along the
    /// `y→to_y` and `x→to_x` axes.
    ///
    /// Returns a tuple with (y, x, to_y, to_x)
    ///
    /// An NcVisual of `y` by `x` pixels will require
    /// (`y` * `to_y`) by (`x` * `to_x`) cells for full output.
    ///
    /// Errors on invalid blitter in `options`. Scaling is taken into consideration.
    ///
    /// *C style function: [ncvisual_blitter_geom()][crate::ncvisual_blitter_geom].*
    pub fn geom(
        &self,
        nc: &Notcurses,
        options: &NcVisualOptions,
    ) -> NcResult<(NcDim, NcDim, NcDim, NcDim)> {
        let mut y = 0;
        let mut x = 0;
        let mut to_y = 0;
        let mut to_x = 0;

        let res = unsafe {
            crate::ncvisual_blitter_geom(
                nc,
                self,
                options,
                &mut y,
                &mut x,
                &mut to_y,
                &mut to_x,
                null_mut(),
            )
        };
        error![
            res,
            "NcVisual.geom()",
            (y as NcDim, x as NcDim, to_y as NcDim, to_x as NcDim)
        ];
    }

    /// Gets the default media (not plot) blitter for this environment when using
    /// the specified scaling method.
    ///
    /// Currently, this means:
    /// - if lacking UTF-8, NCBLIT_1x1.
    /// - otherwise, if not NCSCALE_STRETCH, NCBLIT_2x1.
    /// - otherwise, if sextants are not known to be good, NCBLIT_2x2.
    /// - otherwise NCBLIT_3x2 NCBLIT_2x2 and NCBLIT_3x2 both distort the original
    ///   aspect ratio, thus NCBLIT_2x1 is used outside of NCSCALE_STRETCH.
    ///
    /// *C style function: [ncvisual_media_defblitter()][crate::ncvisual_media_defblitter].*
    pub fn media_defblitter(nc: &Notcurses, scale: NcScale) -> NcBlitter {
        unsafe { crate::ncvisual_media_defblitter(nc, scale) }
    }

    /// Polyfills at the specified location using `rgba`.
    ///
    /// *C style function: [ncvisual_polyfill_yx()][crate::ncvisual_polyfill_yx].*
    pub fn polyfill_yx(&mut self, y: NcDim, x: NcDim, rgba: NcRgba) -> NcResult<()> {
        error![
            unsafe { crate::ncvisual_polyfill_yx(self, y as i32, x as i32, rgba) },
            &format!["NcVisual.polyfill_yx({}, {}, {})", y, x, rgba]
        ]
    }

    /// Renders the decoded frame to the specified [NcPlane].
    ///
    /// See [`NcVisualOptions`].
    ///
    /// *C style function: [ncvisual_render()][crate::ncvisual_render].*
    pub fn render(
        &mut self,
        nc: &mut Notcurses,
        options: &NcVisualOptions,
    ) -> NcResult<&mut NcPlane> {
        error_ref_mut![
            unsafe { crate::ncvisual_render(nc, self, options) },
            "NcVisual.render()"
        ]
    }

    /// Resizes the visual to `rows` X `columns` pixels.
    ///
    /// This is a lossy transformation, unless the size is unchanged.
    ///
    /// *C style function: [ncvisual_resize()][crate::ncvisual_resize].*
    pub fn resize(&mut self, rows: NcDim, cols: NcDim) -> NcResult<()> {
        error![
            unsafe { crate::ncvisual_resize(self, rows as i32, cols as i32) },
            &format!["NcVisual.resize({}, {})", rows, cols]
        ]
    }

    /// Rotates the visual `rads` radians.
    ///
    /// Only M_PI/2 and -M_PI/2 are supported at the moment,
    /// but this will change. (FIXME)
    ///
    /// *C style function: [ncvisual_rotate()][crate::ncvisual_rotate].*
    pub fn rotate(&mut self, rads: f64) -> NcResult<()> {
        error![
            unsafe { crate::ncvisual_rotate(self, rads) },
            &format!["NcVisual.rotate({})", rads]
        ]
    }

    /// Sets the specified pixel.
    ///
    /// *C style function: [ncvisual_set_yx()][crate::ncvisual_set_yx].*
    pub fn set_yx(&mut self, y: NcDim, x: NcDim, pixel: NcPixel) -> NcResult<()> {
        error![
            unsafe { crate::ncvisual_set_yx(self, y as i32, x as i32, pixel) },
            &format!["NcVisual.set_yx({}, {}, {})", y, x, pixel]
        ]
    }

    /// Displays frames.
    ///
    /// *Provide as an argument to ncvisual_stream().*
    ///
    /// If you'd like subtitles to be decoded, provide an ncplane as the curry.
    /// If the curry is NULL, subtitles will not be displayed.
    ///
    /// *C style function: [ncvisual_simple_streamer()][crate::ncvisual_simple_streamer].*
    pub fn simple_streamer(
        &mut self,
        options: &mut NcVisualOptions,
        time: &NcTime,
        curry: Option<&mut NcPlane>,
    ) -> NcResult<()> {
        if let Some(plane) = curry {
            error![
                unsafe {
                    crate::ncvisual_simple_streamer(
                        self,
                        options,
                        time,
                        plane as *mut _ as *mut libc::c_void,
                    )
                },
                &format![
                    "NcVisual.simple_streamer({:?}, {:?}, ncplane)",
                    options, time
                ]
            ]
        } else {
            error![
                unsafe { crate::ncvisual_simple_streamer(self, options, time, null_mut()) },
                &format!["NcVisual.simple_streamer({:?}, {:?}, null)", options, time]
            ]
        }
    }

    // // TODO
    //
    // /// Streams the entirety of the media, according to its own timing.
    // ///
    // /// Blocking, obviously.
    // ///
    // /// If `streamer` is provided it will be called for each frame, and its return
    // /// value handled as outlined for streamcb.
    // /// If streamer() returns non-zero, the stream is aborted, and that value is
    // /// returned.  By convention, return a positive number to indicate intentional
    // /// abort from within streamer().
    // ///
    // /// `timescale` allows the frame duration time to be scaled.
    // /// For a visual naturally running at 30FPS, a 'timescale' of 0.1 will result
    // /// in 300 FPS, and a `timescale` of 10 will result in 3 FPS.
    // /// It is an error to supply `timescale` less than or equal to 0.
    // ///
    // /// *C style function: [ncvisual_streamer()][crate::ncvisual_streamer].*
    // //
    // // TODO: add streamcb
    // // INFO: QUESTION: is curry also optional like in simple_streamer?
    // //
    // pub fn simple_streamer(
    //     &mut self,
    //     nc: &mut Notcurses,
    //     timescale: f32,
    //     //streamer: Option<streamcb>
    //     options: &NcVisualOptions,
    //     curry: Option<&mut NcPlane>,
    // ) -> NcResult<()> {
    // }

    /// If a subtitle ought be displayed at this time, returns a heap-allocated
    /// copy of the UTF8 text.
    ///
    /// *C style function: [ncvisual_subtitle()][crate::ncvisual_subtitle].*
    pub fn subtitle(&self) -> NcResult<&str> {
        let res = unsafe { crate::ncvisual_subtitle(self) };
        if !res.is_null() {
            Ok(rstring![res])
        } else {
            Err(NcError::with_msg(NCRESULT_ERR, "NcVisual.subtitle()"))
        }
    }
}

/// # NcDirectF Constructors & destructors
impl NcDirectF {
    /// Loads media from disk, but do not yet renders it (presumably because you
    /// want to get its geometry via [ncdirectf_geom()][0], or to use the same
    /// file with [ncdirectf_render()][1] multiple times).
    ///
    /// You must destroy the result with [ncdirectf_free()][2];
    ///
    /// [0]: crate::NcDirectF#method.ncdirectf_geom
    /// [1]: crate::NcDirectF#method.ncdirectf_render
    /// [2]: crate::NcDirectF#method.ncdirectf_free
    ///
    /// *C style function: [ncdirectf_from_file()][crate::ncdirectf_from_file].*
    pub fn ncdirectf_from_file<'a>(ncd: &mut NcDirect, file: &str) -> NcResult<&'a mut NcDirectF> {
        error_ref_mut![
            unsafe { crate::ncdirectf_from_file(ncd, cstring![file]) },
            &format!("NcDirectF::ncdirectf_from_file(ncd, {})", file)
        ]
    }

    /// Frees a [`NcDirectF`] returned from [ncdirectf_from_file()][0].
    ///
    /// [0]: crate::NcDirectF#method.ncdirectf_from_file
    ///
    /// *C style function: [ncdirectf_free()][crate::ncdirectf_free].*
    pub fn ncdirectf_free(&mut self) {
        unsafe { crate::ncdirectf_free(self) };
    }
}

/// # NcDirectF Methods
impl NcDirectF {
    /// Same as [`NcDirect.render_frame()`][0], except `frame` must already have
    /// been loaded.
    ///
    /// A loaded frame may be rendered in different ways before it is destroyed.
    ///
    /// [0]: NcDirect#method.render_frame
    ///
    /// *C style function: [ncvisual_render()][crate::ncvisual_render].*
    pub fn ncdirectf_render(
        &mut self,
        ncd: &mut NcDirect,
        blitter: NcBlitter,
        scale: NcScale,
        max_y: NcDim,
        max_x: NcDim,
    ) -> NcResult<&mut NcDirectV> {
        error_ref_mut![
            unsafe {
                crate::ncdirectf_render(ncd, self, blitter, scale, max_y as i32, max_x as i32)
            },
            "NcVisual.render()"
        ]
    }
    /// Having loaded the `frame`, get the geometry of a potential render.
    ///
    /// *C style function: [ncdirectf_geom()][crate::ncdirectf_geom].*
    pub fn ncdirectf_geom(
        &mut self,
        ncd: &mut NcDirect,
        blitter: &mut NcBlitter,
        scale: NcScale,
        max_y: NcDim,
        max_x: NcDim,
    ) -> NcResult<NcVGeom> {
        let mut geom = NcVGeom::new();

        let res = unsafe {
            crate::ncdirectf_geom(
                ncd,
                self,
                blitter,
                scale,
                max_y as i32,
                max_x as i32,
                &mut geom,
            )
        };
        error![res, "NcDirectF.ncdirectf_geom()", geom];
    }
}

/// # NcVGeom Constructors
impl NcVGeom {
    /// Returns a new `NcVGeom` with zeroed fields.
    pub fn new() -> Self {
        Self {
            pixy: 0,
            pixx: 0,
            cdimy: 0,
            cdimx: 0,
            rpixy: 0,
            rpixx: 0,
            rcelly: 0,
            rcellx: 0,
            scaley: 0,
            scalex: 0,
            maxpixely: 0,
            maxpixelx: 0,
        }
    }
}

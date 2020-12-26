use crate::{NcChannelPair, NcMenuOptions, NcMenuSection};

/// # `NcMenuOptions` constructors
impl NcMenuOptions {
    /// New NcMenuOptions.
    pub fn new(sections: Vec<NcMenuSection>) -> Self {
        Self::with_all_args(sections, 0, 0, 0)
    }

    /// New empty NcMenuOptions.
    pub fn new_empty() -> Self {
        Self::with_all_args(vec![], 0, 0, 0)
    }

    /// New NcMenuOptions with width options.
    pub fn with_all_args(
        mut sections: Vec<NcMenuSection>,
        headerc: NcChannelPair,
        sectionc: NcChannelPair,
        flags: u64,
    ) -> Self {
        Self {
            // array of 'sectioncount' `MenuSection`s
            sections: sections.as_mut_ptr() as *mut NcMenuSection,

            //
            sectioncount: sections.len() as i32,

            // styling for header
            headerchannels: headerc,

            // styling for sections
            sectionchannels: sectionc,

            // flag word of NCMENU_OPTION_*
            flags: flags,
        }
    }
}

/// # `NcMenuOptions` methods
impl NcMenuOptions {
    /// Returns the styling for the header.
    ///
    /// *(No equivalent C style function)*
    pub const fn header_channels(&self) -> NcChannelPair {
        self.headerchannels
    }

    /// Returns a mutable reference of the styling for the sections.
    ///
    /// *(No equivalent C style function)*
    pub fn header_channels_mut<'a>(&'a mut self) -> &'a mut NcChannelPair {
        &mut self.headerchannels
    }

    /// Returns the styling for the sections.
    ///
    /// *(No equivalent C style function)*
    pub const fn section_channels(&self) -> NcChannelPair {
        self.sectionchannels
    }

    /// Returns a mutable reference of the styling for the sections.
    ///
    /// *(No equivalent C style function)*
    pub fn section_channels_mut<'a>(&'a mut self) -> &'a mut NcChannelPair {
        &mut self.sectionchannels
    }
}

fn main() {
    use clap::{load_yaml, App};
    let yaml = load_yaml!("cli.yml");
    let matches = App::from_yaml(yaml).get_matches();
    if matches.is_present("msgbox") {
        // do a messagebox
    }else{
        eprintln!("Needed a widget type");
        std::process::exit(1);
    }
}

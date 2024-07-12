
# jwms
~~A WIP poor mans session manager for JWM (Joe's Window Manager)~~

A configuration generator for JWM (Joe's Window Manager)

### Current State

jwms implements the parts of XDG menu and icon specification and will populate the root menu with applications/programs you have installed on your system and will try to find the correct icon for each of them.

Menu catagories will be created only if there is at least one program that contains said category.

Styles, key bindings, autostart script, and the tray are also created and are mostly finished.

When ran, based on how you configured the jwms.conf file. jwms will generate the `.jwmrc` file in the home directory along with its required files in the `/home/USERNAME/.config/jwm` directory.

### Configuring jwms.conf

jwms offers a much easier way of configuring jwm by using libconfuse for handling the configuration.

Currently, pleae refer to the provided jwms.conf in the repo if you need a complete example.

A guide is going to be provided eventually.

### Building and running

Requires libconfuse and libbsd

Run `make` in the project root directory to create the binary

You should now be able run the program via `./jwms --all` which creates all files needed for JWM

You can also specify what parts to generate or not by doing `./jwms --help`

For example, doing `./jwms --menu` will only create the JWM root menu file.

You can add as many arguments as you want, which means you can do `./jwms --menu --binds --icons --tray` which creates the root menu, keybinds, icon paths, and the tray while ignoring the rest.

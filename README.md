
# jwms
A simple session manager + configuration generator for JWM (Joe's Window Manager)

### Current State

This includes two programs. jwm-helper and jwms

jwm-helper implements the parts of XDG menu and icon specification and will populate the root menu with applications/programs you have installed on your system and will try to find the correct icon for each of them.

Menu catagories will be created only if there is at least one program that contains said category.

Styles, key bindings, autostart script, and the tray are also created and are mostly finished.

When ran, based on how you configured the jwms.conf file. jwm-helper will generate the `.jwmrc` file in the home directory along with its required files in the `/home/USERNAME/.config/jwm` directory.

The jwms program is a very simple daemon that runs in the background when it gets called from the Display Manager.

Its job is to run jwm-helper and start jwm. That's it (for now).

### Configuring jwms.conf

jwm-helper offers a much easier way of configuring jwm by using libconfuse for handling the configuration.

Currently, pleae refer to the provided jwms.conf in the repo if you need a complete example.

A guide is going to be provided eventually.

### Building and running

Requires libconfuse, libbsd and libx11

Run `make` in the project root directory to create the binaries.

Before running, make sure there is a valid `.gtkrc-2.0` file in your home directory. The jwm-helper needs this to get the current icon theme. (I know, it kinda sucks)

You should now be able run the jwm-helper program via `./jwm-helper --all` which creates all files needed for JWM

You can also specify what parts to generate or not by doing `./jwm-helper --help`

For example, doing `./jwm-helper --menu` will only create the JWM root menu file.

You can add as many arguments as you want, which means you can do `./jwm-helper --menu --binds --icons --tray` which creates the root menu, keybinds, icon paths, and the tray while ignoring the rest.

The jwms program is not meant to be run by the user, that is handled by the Display Manager.

### Installing

Run the following commands to do a system install.

`make`

`sudo make install` 

You should now have both binaries installed in `/usr/local/bin` and the default config that is provided in `/etc/jwms` along with the xsession desktop file in `/usr/share/xsessions`

Don't use the `install.sh` script if you are building from source. That's for binary releases only.

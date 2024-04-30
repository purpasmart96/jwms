# jwms
A WIP poor mans session manager for JWM (Joe's Window Manager)

### Current State

jwms implements the parts of XDG menu and icon specification and will populate the root menu with applications/programs you have installed on your system and will try to find the correct icon for each of them.

Styles, bindings, and the tray is also created but is still mostly hardcoded for now. When ran, jwms will generate a .jwmrc file based on the jwms.conf file you have configured or it will use default values. 

### Build instructions

Requires libconfuse and libbsd

Run 'make' in the project root directory to create the binary

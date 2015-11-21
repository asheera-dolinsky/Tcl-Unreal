# UETclWrapper
This is a simple Tcl wrapper for the Unreal Engine implemented as a fully static class.
It's the first good effort to implement Tcl into the Unreal Engine.
Making it into a plugin is planned later once it matures.

### Bootstrapping
* clone the [Tcl repo](https://github.com/tcltk/tcl)
* run the **VS2015 x64 Native Tools Command Prompt** (use x32 version if you're building your UE project for x86_32 arch)
* navigate to the directory you cloned Tcl and then to its *win* sub-directory
* run **buildall.vc.bat**, it will cry "BOOM!" because Tk is not present, but we don't need it.
* copy the Tcl dll into the sub-directory called *ThirdParty* in the root of your UE project
* copy *tcl.h*, *tclDecls.h*, *tclPlatDecls.h* from the Tcl repo into the src of this project
* put that src somewhere you can easily include it from into your UE project
* possibly you will need to adjust the **_TCL_DLL_FNAME_** constant in *TclWrapper.hpp* if it's not *"tcl86t.dll"*
* include the *TclWrapper.hpp* wherever you need it

### Usage
* use Tcl_Interp* TclWrapper::bootstrap() to bootstrap everything once and only once and get a new interpreter instance
* use int TclWrapper::eval(Tcl_Interp*, const char*) to run a Tcl script, it returns TCL_OK or a specific error code
* use void TclWrapper::registerFunction(Tcl_Interp*, const char*, **callback**, ClientData, Tcl_CmdDeleteProc*);
* **callback** is basically a wrapper for whatever function you want and has the following signature: int (ClientData, Tcl_Interp*, int, Tcl_Obj *const*)
* the last two parameters of that signature are nullable, but need to be cast into their respective types like this: (ClientData) NULL, (Tcl_CmdDeleteProc*) NULL
* for more information on embedding and Tcl refer to the official Tcl documentation, luckily it's actually very very good

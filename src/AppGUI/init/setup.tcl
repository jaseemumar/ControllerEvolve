< init/gui.tcl

#setup the location of the console and the toolbar

# laptop settings 

set laptop 0

if {$laptop == 1} {
	puts "laptop = 1"
	#wm geometry . =1017x700
	console eval {wm geometry . =122x10}
	launch ControllerEditor 1000 500
	finalizeUI
	wm title .dialog "Control Parameters"
	wm geometry .dialog =260x120
} else {
	puts "laptop = 0"
	#wm geometry . =1408x700
	console eval {wm geometry . =172x15}
	launch ControllerEditor 1400 700
	finalizeUI
	wm title .dialog "Control Parameters"
	wm geometry .dialog =260x120
}


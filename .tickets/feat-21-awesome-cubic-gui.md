Be awesome to have the GUI like a 3D matrix that can be turned with hotkeys.

So pressing some modifier, let's say CTRL, + an arrow key will turn the cube.

So first you see 1 face of the cube. Let's call this the front face. The width of this face is the length in time of all sequences. The height here is divided in rows, each row being 1 of the parallel sequences.

Now if you would press CTRL+right, the cube will turn so that you see the face to the right of this one. It might have a fancy animation while doing so or not. Anyway, this right face of the cube shows per sequence a kind of editable graph representing the successive parameter values for the selected parameter of that sequence. So if the selected parameter is for example frequency, then this right face of the cube might show an ascending graph of discrete frequency points that may be clicked and dragged to edit them (or may also be edited using the keyboard, as before).

If you now press (CTRL+left) 2 times, then you first go back to the front face and then the left face of the cube, which may show something else eventually, but for now let's just focus on implementing the front and right face views first.

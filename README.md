## Clearbench
A comparison against different memory clearing strategies for the PS2 Graphics Synthesizer

I'm happy to include other strategies if any are suggested.

#### BigSprite
##### "The naive approach"
Simply draws a large 640x448 sprite.

#### Double Half Clear
##### Set the zbuf to the middle of the frame, clear the top half.
The zbuffer will clear the bottom half. Because depth testing is free, you only have to rasterize half of the amount of pixels.

#### Page Clear
##### Clear the framebuffer in 64x32 pages
Instead of causing multiple page breaks, fill in memory a page at a time.

#### Page Clear (With DHC)
##### The two previous tests together
Straightforward, a Double Half Clear on the top half of the frame while the zbuffer clears the bottom.

#### Interleaved Clear
##### FBP=ZBP and interleave clear by drawing 32x32 sprites
Because of the mirrored block arrangement in CT32/CT24 vs Z32/Z24, filling in the front half of a page will be mirrored by the zbuffer write, filling in the other half.

#### VIS Clear
##### FBW = 1 and tall 64x2048 sprites
Setting FBW 1 essentially stacks the framebuffer vertically in 64x32 pages.

#### VIS Clear (Interleaved)
##### The VIS clear with an interleaved approach
Same configuration as the VIS Clear, but now we set ZBP to FBP. Now we only have to draw 32x2048 sprites.


## Credits
Thank you to refractionpcsx2 and PSI's [PS2TEK](https://psi-rockin.github.io/ps2tek/#gsspecialeffects) for the support and documentation required to preserve these fun optimizations.

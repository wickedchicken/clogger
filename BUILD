right now it's easy to build! just run

> cd build/
> cmake ../
> make clean && make && sudo checkinstall make install

if you want to use a different compiler (such as clang) do this:

> CC=clang cmake ../

if your changes aren't being picked up by cmake then remove CMakeCache.txt

also, you don't really need checkinstall but it makes your life incredibly easier!

I wouldn't recommend building in-tree since cmake generates a lot of files and it'd
be annoying to track them all down.

enjoy!
- mike

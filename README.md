# BOSS
Build Order Search System (**BOSS**) allows for real-time build order planning for the game of StarCraft II. It can be used as a tool to find optimal (given a function) build orders, or can be combined with a bot player (see [CommandCenter + BOSS](https://github.com/ArtaSeify/CommandCenter)). Currently, only Depth First Search is supported, as well as only the Protoss race; this repo is a work in progress.

BOSS finds the optimal build order that maximizes the area (integral) under a given function. It is based on the concepts presented in the paper by [Dave Churchill, Michael Buro, Richard Kelly](http://www.cs.mun.ca/~dchurchill/pdf/cog19_buildorder.pdf). Dave Churchill is a co-author of this repo, and his StarCraft I BOSS repo will be linked to once available. The previous version of BOSS for StarCraft I can be found as a module in [UAlbertaBot](https://github.com/davechurchill/ualbertabot).

Requirements:
Visual Studio 2019 (not tested with previous versions)
No external libraries required. If you want to import additional libraries, put them inside /lib and /include in the directory.

Note that this project is a work in progress:
- only CombatSearch has been thoroughly tested.
- Monte-Carlo Tree Search will be coming.
- The goal is to implement all races, though only Protoss is currently implemented. 

More detailed documentation is a work in progress.

# How to contribute to the FlaxEngine

For any questions, suggestions or help join our discord!

<a href="https://flaxengine.com/discord"><img src="https://discordapp.com/api/guilds/437989205315158016/widget.png"/></a>

Want to see whats planned for Flax?

Go check out our [Trello](https://trello.com/b/NQjLXRCP/flax-roadmap).

## **Found a bug?**

* Avoid opening any new issues without having checked if your problem has already been reported. If there are no currently open issues that fit your problem's description, feel free to [add it](https://github.com/FlaxEngine/FlaxEngine/issues/new).

* When writing an issue make sure to include a clear title and description as well as having filled out all the necessary information, depending on the severity of the issue also include the necessary log files and minidump.

* Try to following the given template when writing a new issue if possible.

## **Want to contribute?**
  
* Fork the FlaxEngine, create a new branch and push your changes there. Then, create a pull request.
  
* When creating a PR for fixing an issue/bug make sure to describe as to what led to the fix for better understanding, for small and obvious fixes this is not really needed. 
  However make sure to mention the relevant issue where it was first reported if possible.
  
* For feature PR's the first thing you should evaluate is the value of your contribution, as in, what would it bring to this engine? Is it really required?
  If its a small change you could preferably suggest it to us on our discord, else feel free to open up a PR for it.
  
* Ensure when creating a PR that your contribution is well explained with a adequate description and title.

* Generally, good code quality is expected, make sure your contribution works as intended and is appropriately commented where necessary.


Thank you for taking interest in contributing to Flax!

## **Common issues**

Below are some common issues that someone working with the FlaxEngine source code might run into. Hopefully some of those issues will get fixed in the future. If you know how, please contribute!

* Missing MSVC toolset
  * Install it through the Visual Studio Installer
* Building or attaching fails
  * Run `GenerateProjectFiles.bat`
  * Rebuild `Flax.Build`
  * Make sure that there isn't a stray FlaxEngine process running in the background
  * First start Flax and then attach the C# debugger
  * Configure the C# FlaxEngine project by going into the project properties, then the debug tab and selecting "Start external program" `Flax\FlaxEngine\Binaries\Editor\Win64\Debug\FlaxEditor.exe`
    * Then you can also set command line arguments such as `-project "C:\Users\PROFILE\Documents\Flax Projects\FlaxSamples\BasicTemplate"`
* Git LFS
  * Push with `git push --no-verify`

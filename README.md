If you need to find the logic for specific parts of the project, go to these files:

parsing/parser.cpp & .hpp: This contains the code that takes raw text from the network and breaks it into a Command object (Prefix, Command Name, and Parameters).

parsing/cmdDispatcher.cpp & .hpp: This is the "brain" of the commands. It checks the command name (PASS, NICK, USER, etc.) and runs the correct function for it.

parsing/Client.cpp & .hpp: This stores all user data, such as their nickname, username, and whether they have successfully logged in.

How the Code Works
The server follows a specific order for new users:

PASS: The user must send the password you set when starting the server.

NICK: The user chooses a name. The code checks if the name is valid and not already taken.

USER: The user provides their final details. Once this is done, they are "Registered" and can join channels.

To add a new command: Simply create a new handler function and add it to the execute list in cmdDispatcher.cpp.

Git Instructions
To avoid code conflicts and lost work, please follow these rules:

Clone the project:
git clone <repo-url>

Use your own branch:
Never work on the master branch. Create your own:
git checkout -b your-name-branch

Pull before you start:
Every time you sit down to code, run this command first:
git pull origin master
This ensures you have the latest code from everyone else before you add your own.

to push your edited files you should use:
for example: git push -u origin "your branch name"

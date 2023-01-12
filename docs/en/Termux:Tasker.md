This add-on provides a Tasker plug-in to execute Termux programs.

## Installation

Download add-on from
[F-Droid](https://f-droid.org/packages/com.termux.tasker/)

**Important: Do not mix installations of Termux and Addons between
Google Play and F-Droid.** They are presented at these portals for your
convenience. There are compatibility issues when mixing installations
from these Internet portals. This is because each download website uses
a specific key for keysigning Termux and [Addons](Addons).

## Usage

Create a new Tasker Action. In the resulting Select Action Category
dialog, select Plugin. In the resulting dialog, select Termux:Tasker.
Edit the configuration to specify the executable in \~/.termux/tasker/
to execute, and if it should be executed in the background (the default)
or in a new terminal session.
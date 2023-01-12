As of July 28 2019 packages ending in **-dev** will no longer exist.

These packages previously contained header files and other files needed
to build software on the device. As those files are small they are no
longer split out but instead merged into their main packages.

# Example: ncurses

Previously the package **ncurses-dev** contained the header files to
compile code using ncurses. This package no longer exists and the header
files are now a part of the **ncurses** package, along with the shared
libraries there.

# What to do?

- Run **pkg up** to update as normally. This will replace all **-dev**
  packages with updated versions of main packages.
- If you are trying to execute scripts or follow a tutorial that tries
  to install a **-dev** package, just remove the **-dev** suffix.
- If you have further questions, see the
  [Community](Community) pages for where to ask questions.
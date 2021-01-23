# Termux

[Termux](termux.com) é um emulador de terminal e ambiente Linux para Android.

- [Comunidade no Reddit](https://reddit.com/r/termux)
- [Wiki](https://wiki.termux.com/wiki/)
- [Twitter](http://twitter.com/termux/)

Note que este repositório é apenas para o **aplicativo** Termux (a interface e a emulação do terminal).
Para os pacotes instaláveis dentro do aplicativo, veja https://github.com/termux/termux-packages
***
**@termux está procurando por um desenvolvedor para implementar novas *features*, consertar *bugs* e analizar *pull requests*, já que o atual (@fornwall) está inativo.**
**A *issue* https://github.com/termux/termux-app/issues/1072 precisa de atenção extra.**
***

## Instalação

O Termux pode ser obtido em:

- [Google Play](https://play.google.com/store/apps/details?id=com.termux)
- [F-Droid](https://f-droid.org/en/packages/com.termux/)
- [Kali Nethunter Store](https://store.nethunter.com/en/packages/com.termux/)

Além disso, nós oferecemos *builds* “beta” para aqueles que querem experimentar as últimas *features* ou testar seus *pull requests*.
Esta *build* pode ser obtida de uma das *workflow runs* listadas em [Github Actions](https://github.com/termux/termux-app/actions)

As chaves de assinatura de todas as *builds* oferecidas são diferentes. Antes de trocar a fonte de instalação, você deverá desinstalar o Termux e todos os *plug-ins* instalados.

## Recursos do terminal

- [Sequências de controle](http://invisible-island.net/xterm/ctlseqs/ctlseqs.html)
- [vt100.net](http://vt100.net/)
- [Códigos do terminal (ANSI e equivalentes do terminfo)](http://wiki.bash-hackers.org/scripting/terminalcodes)

## Emuladores de terminal

- VTE (libvte): *widget* de emulador de terminal para GTK+, usado principalmente no *gnome-terminal*. [Fonte](https://github.com/GNOME/vte), [*Issues* abertas](https://bugzilla.gnome.org/buglist.cgi?quicksearch=product%3A%22vte%22+),
e [Todas as (incluindo as fechadas) *issues*](https://bugzilla.gnome.org/buglist.cgi?bug_status=RESOLVED&bug_status=VERIFIED&chfield=resolution&chfieldfrom=-2000d&chfieldvalue=FIXED&product=vte&resolution=FIXED).

- iTerm 2: Aplicativo de terminal para OS X. [Fonte](https://github.com/gnachman/iTerm2), [*Issues*](https://gitlab.com/gnachman/iterm2/issues) e [Documentação](http://www.iterm2.com/documentation.html)
(inclui [códigos de escape proprietários do iTerm2](http://www.iterm2.com/documentation-escape-codes.html)).

- Konsole: Aplicativo de terminak para KDE. [Fonte](https://projects.kde.org/projects/kde/applications/konsole/repository), em particular [testes](https://projects.kde.org/projects/kde/applications/konsole/repository/revisions/master/show/tests), [*bugs*](https://bugs.kde.org/buglist.cgi?bug_severity=critical&bug_severity=grave&bug_severity=major&bug_severity=crash&bug_severity=normal&bug_severity=minor&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole)
e [Desejos](https://bugs.kde.org/buglist.cgi?bug_severity=wishlist&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole).

- hterm: Implementação de terminal JavaScript do Chromium. [Fonte](https://github.com/chromium/hterm), incluindo [testes](https://github.com/chromium/hterm/blob/master/js/hterm_vt_tests.js) e [Google group](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-hterm).

- xterm: O pai dos emuladores de terminal. [Fonte](http://invisible-island.net/datafiles/release/xterm.tar.gz).

- Connectbot: cliente SSH Android. [Fonte](https://github.com/connectbot/connectbot).

- Android terminal emulator: aplicativo de terminal Android no qual o gerenciamento de terminal do Termux é baseado.  Inativo. [Fonte](https://github.com/jackpal/Android-Terminal-Emulator).

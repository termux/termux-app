lorie xserver实现的主要思路
从xserver/dix/mian.c 的mian方法启动xserver
这个过程lorie做了下面几件事
1，实现InitInput事件向xserver注册输入事件，比如鼠标，键盘，书写笔，入口是ininput.c/InitIninput函数
2，实现InitOutput事件，接受xserver的输出事件，主要是屏幕内容输出，其中设置了一个定时器，利用文件描述符监听，入口是是output.c/InitOutput函数
屏幕是否有刷新数据，一旦发现文件描述符可读，立即回调画面刷新函数，具体调用链为
xserver/dix/main.c/dix_main->InitOutput->renderer_init->->eglMakeCurrent->bind screen to GL Surface
                                       ->AddScreen->lorieScreenInit->timerfd_create->lorieTimerCallback->renderer_redraw

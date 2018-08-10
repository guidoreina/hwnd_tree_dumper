hwnd\_tree\_dumper
==================
Command-line tool for Windows for dumping the window tree.

Example of output:
```
/- [0x10010] Class name: '#32769' (Desktop)
    |
    +-- [0x10062] Class name: 'tooltips_class32', exe: 'C:\Windows\Explorer.EXE'
    |
    +-- [0x10066] Class name: 'tooltips_class32', exe: 'C:\Windows\Explorer.EXE'
    |
    +-- [0x10068] Class name: 'tooltips_class32', exe: 'C:\Windows\Explorer.EXE'
    |
    +-- [0x1007a] Class name: 'tooltips_class32', exe: 'C:\Windows\Explorer.EXE'
    |
    +-- [0x20050] Class name: 'tooltips_class32', exe: 'C:\Windows\Explorer.EXE'
    |
    +-- [0x1005e] Class name: 'tooltips_class32', exe: 'C:\Windows\Explorer.EXE'
    |
    +-- [0x10060] Class name: 'tooltips_class32', exe: 'C:\Windows\Explorer.EXE'
    |
    +-- [0x20046] Class name: 'Button', text: 'Start', exe: 'C:\Windows\Explorer.EXE'
    |
    +-- [0x20048] Class name: 'Shell_TrayWnd', exe: 'C:\Windows\Explorer.EXE'
    |    |
    |    +-- [0x2003e] Class name: 'TrayNotifyWnd'
    |    |    |
    |    |    +-- [0x20040] Class name: 'TrayClockWClass', text: '1:08 AM'
    |    |    |
    |    |    +-- [0x2004e] Class name: 'TrayShowDesktopButtonWClass'
    |    |    |
    |    |    +-- [0x2001a] Class name: 'SysPager'
    |    |    |    |
    |    |    |    \-- [0x20022] Class name: 'ToolbarWindow32', text: 'User Promoted Notification Area'
    |    |    |
    |    |    +-- [0x20020] Class name: 'ToolbarWindow32', text: 'System Promoted Notification Area'
    |    |    |
    |    |    \-- [0x20024] Class name: 'Button'
    .    .
    .    .
    .    .
```

Compile with:
```
cl hwnd_tree_dumper.c user32.lib
```

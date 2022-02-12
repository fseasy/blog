---
layout: post
title: 拒绝 bash-completion 
date: 2022-02-11
categories: 技术 
tags: ubuntu bash 自动补全 bash-completion
---
> 你是否在 Ubuntu Bash 上遇到奇怪的自动补全问题，关闭它或许是个好选择……

```bash
$ echo $BASH_VERSION 
5.0.17(1)-release
```

Bash 有原生的补全策略， 看起来就是：
1. 无命令输入时，按 `TAB` 展示所有命令
2. 有命令输入时，展示给定路径（默认为当前文件夹）下所有文件和文件夹名

在这之上，有“第三方”<sup>*</sup>开发的 `bash-completion` 命令（有[github][bc_github]），对原生的补全有提升，
譬如对git，输入 `git` 后按`TAB`, 
会发现提示的内容就是git的一级命令，似乎比默认的列出当前文件夹的所有文件强了点。

> “第三方”的说法，来自 [Bash will not auto-complete (Tab) with files][1] 里答案的描述，未探究真实性。

然而，我实际使用下来，却觉得这个东西只提供了负面作用。具体下来，有2个问题：

1. `bash-completion` 对 Python 脚本的参数输入有负面作用

    比如， 运行自己开发的脚本时，要在`-d`之后键入一个地址：

    ```bash
    $ python build_network.py -d ../output/<TAB><TAB><TAB>...
    ```

    你会发现死活不能列出`../output`下的文件了（里面只有文件）。如果我把 `-d` 换成 `--data`, 就可以

    ```bash
    $ python build_network.py --data ../output/<TAB>
    has_category.wiki.jsonl  wiki_page.jsonl 
    ```

    猜测，`bash-completion` 将 `-d` 限定为 `dir` 了，所以自动拒绝了下面文件的列出。 
    只是，这个限定属于自作聪明。

    其他第三方命令，应该都有类似的问题。

2. `bash-completion` 对环境变量的扩展有负面作用

    比如我要输出一个文件的绝对地址，供`scp`使用，我一般这么操作：

    ```bash
    $ echo $HOSTNAME:$PWD/<TAB>fname
    ```

    一般(如在 CentOS6.3 下)，
    在按下`TAB`之后，前面 `$PWD` 就会自动展开，然后会给出列出当前文件夹下的文件，方便补全具体文件名。
    然而，在 `bash-completion` 的控制下，上面的结果成为

    ```bash
    $ echo $HOSTNAME:$PWD/<TAB>
    => 
    $ echo $HOSTNAME:\$PWD/
    ```

    它把 `$PWD` 给转义了…… 真是被气坏了。
    虽然，你也可以使用 `Ctrl + Alt + E` 来在`Tab`前把环境变量扩展开来，但是体验上还是低了点

如何解决此问题？ 关闭它，比较简单……

在[disable-bashs-programmable-autocompletion-based-on-command][2]中，有一些答案，下面2种都行：

1. `shopt -u progcomp` 
    
    `shopt` （即 shell-option）是设置bash的， `-u` 为禁用此选项。
    此操作只对当前终端有效。如需每次生效，可以放到 `.bashrc` 中。

2. `complete -r`

    从 [What's the use of complete command?][3] 看到，
    `complete` 和 `bash-completion` 基本是绑定的，相当于动态决定了哪些命令要使用 `bash-completion`。

    看起来可以基于此来特定的取消掉某些命令的自动补全？
    但可惜的是， 我用 `complete -r python` 并不能取消 Python 脚本的自动补全，用 `complete` 打印所有的规则，
    也并无 Python. 或许上述的限定，是一个默认规则吧（但是，默认规则不应该是保守的全部列出吗？不懂了）。
    所以，只有 `complete -r` 取消掉全部规则。

    此操作同样需要放到`.bashrc` 中才能每次生效。

此外，从 Github 的 README 里的 FAQ 中，我们看到 *Use `M-/` to (in the words of the bash man page) attempt file name completion on the text to the left of the cursor. This will circumvent any file type restrictions put in place by the bash completion code.* 
似乎用 `M-/` (从[1][1]中看就是 `Alt + /`) 可以将当前命令的自动补全禁止掉；然而我自己尝试却没有成功。此法作罢。

最后的最后，我又去搜了下 `zsh` 的自动补全，似乎要比这个智能点？ 我只简单用过，并没有体会到。
`bash-completion` 的设计还是挺有扩展性的，支持第三方应用定义自己的自动补全，然而从搜集资料的过程中看到，
这个功能本身的 bug 还是挺多的，可配置性也差。所以，作为普通的开发，我在找到上述问题的原因后，
毫无心理负担的关闭了`bash-completion`，并不担心它将会带来任何的效率降低。 

[bc_github]: https://github.com/scop/bash-completion
[1]: https://stackoverflow.com/questions/22033261/bash-will-not-auto-complete-tab-with-files
[2]: https://superuser.com/questions/421397/disable-bashs-programmable-autocompletion-based-on-command
[3]: https://askubuntu.com/questions/443186/whats-the-use-of-complete-command
---
layout: post
categories: 工具
tags: mathjax
mathjax: force_enable
date: 2019-01-01
title: MathJax Demo测试
---

<blockquote>
本页面脚本来自 MathJax 在 Github上 的<a href="https://github.com/mathjax/MathJax-demos-web/blob/master/input-tex2chtml.html.md">demo</a>.
</blockquote>

<p>输入Tex</p>

Tips: 

1. 无需 `$$`
2. 换行可以用 Latex 中的 `\begin{split}...\end{split}` 环境，配合 `\\ 和 &`

<div class="row form-group">
  <div class="col-md-12">
    <textarea id="MathInput" class="form-control" rows="6">
    </textarea>
  </div>
</div>


<div class="row form-group">
  <div class="col-md-4 col-md-offset-8 text-right" >
    <input type="checkbox" id="DisplayStyle" checked> <label for="display">渲染带样式</label>
    <button type="button" id="MathRender" class="btn btn-primary">渲染公式</button>
  </div>
</div>

<p>效果预览</p>

<div id="MathShow" style="min-height:100px ; width : 100% ; margin : auto ; border:1px solid #ccc ;border-radius: 4px ; padding: 6px 18px;"></div>

<script>
  function convert() {
    //
    //  Get the TeX input
    //
    var input = document.getElementById("MathInput").value.trim();
    //
    //  Disable the display and render buttons until MathJax is done
    //
    var display = document.getElementById("DisplayStyle");
    var button = document.getElementById("MathRender");
    button.disabled = display.disabled = true;
    //
    //  Clear the old output
    //
    output = document.getElementById('MathShow');
    output.innerHTML = '';
    //
    //  Reset the tex labels (and automatic equation numbers, though there aren't any here).
    //  Get the conversion options (metrics and display settings)
    //  Convert the input to CommonHTML output and use a promise to wait for it to be ready
    //    (in case an extension needs to be loaded dynamically).
    //
    MathJax.texReset();
    var options = MathJax.getMetricsFor(output);
    options.display = display.checked;
    MathJax.tex2chtmlPromise(input, options).then(function (node) {
      //
      //  The promise returns the typeset node, which we add to the output
      //  Then update the document to include the adjusted CSS for the
      //    content of the new equation.
      //
      output.appendChild(node);
      MathJax.startup.document.clear();
      MathJax.startup.document.updateDocument();
    }).catch(function (err) {
      //
      //  If there was an error, put the message into the output instead
      //
      output.appendChild(document.createElement('pre')).appendChild(document.createTextNode(err.message));
    }).then(function () {
      //
      //  Error or not, re-enable the display and render buttons
      //
      button.disabled = display.disabled = false;
    });
  }

  var button = document.getElementById("MathRender");
  button.onclick = function() { convert(); };

</script>

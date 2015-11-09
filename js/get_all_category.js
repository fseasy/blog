---
---
// First , load all post data
{% include postdata.html %}
{{ postdata | strip_newlines | remove: ' ' }}

/* ALL categories : ["作品", "电影", "心情", "机器学习", "总结", "笔记", "算法", "自然语言处理", "工具", 
                     "动漫", "游戏", "书籍", "jekyll", "update"]
*/
var all_categories = [] ;

var post_len = post_data.length ;
for(var i = 0 ; i < post_len ; ++i)
{
    var cur_categories = post_data[i].categories ;
    var len = cur_categories.length ;
    for(var j = 0 ; j < len ; ++j)
    {
        var category_sel = cur_categories[j] ;
        if(all_categories.indexOf(category_sel) == -1) all_categories.push(category_sel) ;
    }
}

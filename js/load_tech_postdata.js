---
---
// First , load all post data
{% include postdata.html %}
{{ postdata | strip_newlines | remove: ' ' }}

/* ALL categories : ["作品", "电影", "心情", "机器学习", "总结", "笔记", "算法", "自然语言处理", "工具", 
                     "动漫", "游戏", "书籍", "jekyll", "update"]
*/
techCategories = ['技术' , '机器学习','笔记','算法','自然语言处理','工具'] ;

techPostData = [] ;

for(var i = 0 ; i < post_data.length ; ++i){
    var curCategories = post_data[i].categories ;
    var isTech = false ;
    for(var j = 0 ; j < curCategories.length ; ++j){
        if(techCategories.indexOf(curCategories[j])){
            isTech = true ;
            break ;
        }
    }
    if(isTech) techPostData.push(post_data[i]) ;
}


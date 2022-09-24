---
---
// 1. Get post data by Jekyll Liquid and store in Template var 
// 2. Assign tmeplate var result to Javascirpt var (unser a namespace)
// -----
// Following, use Jekyll Liquid to generating all post data and store in Liquid template var.
{% capture postdata %}
[
    {% for post in site.posts %}
        {
                "title": "{{ post.title | escape }}" ,
                "url": "{{ post.url | prepend: site.baseurl }}" ,
                "date": '{{ post.date | date: "%Y/%m/%d" }}' ,
                "tags": [ 
                            {% for tag in post.tags %}
                                "{{tag | escape }}"
                                {% if forloop.last == false %}
                                ,
                                {% endif %}
                            {% endfor  %}
                    ],
                "categories": [
                            {% for category in post.categories %}
                                "{{category | escape }}"
                                {% if forloop.last == false %}
                                ,
                                {% endif %}
                            {% endfor  %}
                    ] ,
                "excerpt": "{{ post.excerpt | strip_html }}"
        }
        {% if forloop.last == false %}
        ,
        {% endif %}
    {% endfor %}
]
{% endcapture %}
// Now, assign template var to js var.
var postdataNs = window.postdataNs || {};
postdataNs.posts = {{ postdata | strip_newlines}};

# coding:utf-8
'''
使用 Python 对所有的 html 的文档进行预处理.
主要目的有两个:
	1. 把所有的文档内容整合到一个文件中, 方便后续cpp进行处理
	2. 把所有的html进行去标签
	得到一个 raw_input 文件
	约定的文件格式为:
		1. 每行对应一个 html 文件
		2. 每行中包含三个部分
		a) jump_url, 通过 url 前缀 + 当前文件目录的结构
		b) title, 解析 html, 里面的 title 标签包含着
		c) content(正文), 对 html 内容进行去标签. 也需要把所有的换行去掉
		'''
#封装的操作系统相关的API
import os
import re
import sys
#用来解析html的,将html解析成结构化的数据
from bs4 import BeautifulSoup

reload(sys)
sys.setdefaultencoding('utf8')

input_path = '../data/input/'
output_path = '../data/tmp/raw_input'
url_prefix = 'https://www.boost.org/doc/libs/1_70_0/doc/'


def filter_tags(htmlstr):
    #先过滤CDATA
    re_cdata=re.compile('//<!\[CDATA\[[^>]*//\]\]>',re.I) #匹配CDATA
    re_script=re.compile('<\s*script[^>]*>[^<]*<\s*/\s*script\s*>',re.I)#Script
    re_style=re.compile('<\s*style[^>]*>[^<]*<\s*/\s*style\s*>',re.I)#style
    re_br=re.compile('<br\s*?/?>')#处理换行
    re_h=re.compile('</?\w+[^>]*>')#HTML标签
    re_comment=re.compile('<!--[^>]*-->')#HTML注释
    s=re_cdata.sub('',htmlstr)#去掉CDATA
    s=re_script.sub('',s) #去掉SCRIPT
    s=re_style.sub('',s)#去掉style

    #[注意！！]把br替换成空格
    s=re_br.sub(' ',s)

    s=re_h.sub('',s) #去掉HTML 标签
    s=re_comment.sub('',s)#去掉HTML注释
    #去掉多余的空行
    blank_line=re.compile('\n+')

    #[注意]此处把多个换行替换成一个空格
    s=blank_line.sub(' ',s)

    s=replaceCharEntity(s)#替换实体
    return s

##替换常用HTML字符实体.
#使用正常的字符替换HTML中特殊的字符实体.
#你可以添加新的实体字符到CHAR_ENTITIES中,处理更多HTML字符实体.
#@param htmlstr HTML字符串.
def replaceCharEntity(htmlstr):
    CHAR_ENTITIES={'nbsp':' ','160':' ',
                   'lt':'<','60':'<',
                   'gt':'>','62':'>',
                   'amp':'&','38':'&',
                   'quot':'"','34':'"',}

    re_charEntity=re.compile(r'&#?(?P<name>\w+);')
    sz=re_charEntity.search(htmlstr)
    while sz:
        entity=sz.group()#entity全称，如&gt;
        key=sz.group('name')#去除&;后entity,如&gt;为gt
        try:
            htmlstr=re_charEntity.sub(CHAR_ENTITIES[key],htmlstr,1)
            sz=re_charEntity.search(htmlstr)
        except KeyError:
            #以空串代替
            htmlstr=re_charEntity.sub('',htmlstr,1)
            sz=re_charEntity.search(htmlstr)
    return htmlstr


def enum_files(input_path):
    '''
    把input_path所有html文件的路径都获取到
    '''
    file_list = []
    for basedir, dirnames, filenames in os.walk(input_path):
        for filename in filenames:
            if os.path.splitext(filename)[-1] != '.html':
                continue
            file_list.append(basedir + '/' + filename)
    return file_list

def parse_url(path):
    '''
    预期构造成的　jump_curl 行如：
    https://www.boost.org/doc/libs/1_70_0/doc/html/array.html

    path的格式形如：
    ../data/input/html/about.html

    url_prefix 的格式形如：
    https://www.boost.org/doc/libs/1_70_0/doc/
    '''
    return url_prefix + path[len(input_path):]


def parse_title(html):
    soup = BeautifulSoup(html, 'html.parser')
    return soup.find('title').string


def parse_content(html):
    '''
    1.对正文进行脱标签
    2.去除正文中的换行
    '''
    return filter_tags(html)


def parse_file(path):
    '''
    解析html文件,获取到其中jump_url, title, content
    并返回一个三元组
    '''
    html = open(path).read()
    return parse_url(path), parse_title(html), parse_content(html)


def write_result(result, output_fd):
    '''
    把结果写入到文件中
    '''
    output_fd.write(result[0] + '\3' + result[1] + '\3' + result[2] + '\n')


def run():
    '''
    整个预处理动作的入口函数
    '''
    #1.遍历目录下的所有html文件
    #flie_list是一个列表，里面的每一个元素都是一个文件路径
    file_list = enum_files(input_path)
    with open(output_path, 'w') as output_fd:
        #在没有及时回收的时候，短时间内大量的占用文件描述符,
        #with语句，在文件使用完就立即关闭
        #2.针对每一个html文件，进行解析，过去到其中的jump_url, title, content
        for f in file_list:
            #parse_file 返回的结果语气就是一个三元组
            #jump_url, 标题，　正文
            result = parse_file(f)
            #3.把结果写入到输出文件中
            if result[0] and result[1] and result[2]:#如果是None的对象则不执行
                write_result(result, output_fd)


def test1():
    file_list = enum_files(input_path)
    print file_list

def test2():
    file_list = enum_files(input_path)
    for f in file_list:
        print parse_file(f)

#内置的变量名，表示当前模块的名字
if __name__ == '__main__':
    run()
    #test1()
    #test2()








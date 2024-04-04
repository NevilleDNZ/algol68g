#!/usr/bin/env python2
# -*- coding: utf-8 -*-
end=fi=od=esac=lambda *skip: skip # ¢ switch Bourne's "done" with "od" & add 2¢
try_except=try_finally=None # ¢ usage: end(try_except), end(try_finally) ¢

import os, sys
import re

ignore="""Contents                                                         Index

Chapter 5.  Names                                           129
Contents                                                         Index

INDEX                                                         606"""

re_newpage=re.compile("Chapter [1-9][0-9]*[.]  [A-Z][a-zæ]+\s([A-Za-z][a-z]+\s)*\s+(?P<page>[1-9][0-9]*)")
ignore=u"""Contents\s{10}\s+Index

Appendix A.  Answers                                       [5][0-9]{2}
^\s+[1-9][0-9]*\s*$
^\s+x*iv\s*$
^Contents\s{10}\s+[xvi]+
^CONTENTS\s{10}\s+[xvi]+
INDEX\s{10}\s+[56][0-9]{2}"""

re_ignore=re.compile("|".join([ line.join("^$") for line in ignore.splitlines()]))
re_heading=re.compile("^[1-9][0-9]*([.][1-9][0-9]*)+\s+|^[A-Z]{3,99}|^Chapter [1-9][0-9]*$")
re_heading=re.compile("^[1-9][0-9]*([.][1-9][0-9]*)+\s+|^Chapter [1-9][0-9]*$|^Preface$")
re_chapt=re.compile("^Chapter [1-9][0-9]*$")
re_index=re.compile("[.]  [.]  [.]")
re_ex=re.compile("^Ex [1-9]")
re_page=re.compile("([1-9][0-9]*,|[1-9][0-9]*$|[A1-9][0-9]*[.][1-9][0-9.]*)")
re_a_prefix=re.compile(r"     [(]a[)]")
to_a_prefix=" (a)"
to_a_prefix2="     (a)"
increase3="(a)  cur file:=NIL"

# CONTENTS xiii Bibliography 586 Preface

#re_page=re.compile("^\s+[1-9][0-9]*\s*$")

re_p=re.compile("^( {3,4}([IA] |[A-Z][a-z])|^[A-Za-z]).*\s")
re_hr=re.compile("____*")
re_cont=re.compile("[a-z]-$")
re_stop=re.compile("[.]$")
re_copy=re.compile(r"c &#x25CB;")
re_bull=re.compile(r"&#x2219;")
re_dot=re.compile("[.]")
re_ws=re.compile("\s+")
max_stop=70

indent= " "*30
re_right=re.compile("^\s{25}\s+")

re_special=re.compile("|".join("""^\s{10}\s*Sian Leitch
^\s{10}\s*Inbhir Nis
^\s{10}\s*March 2003
^\s{10}\s*To$
^\s{10}\s*Aad van Wijngaarden
^\s{10}\s*Father of Algol 68""".splitlines()))

import codecs
enc, dec, rwrap, wwrap = codecs.lookup('utf-8')
#enc, dec, rwrap, Wwrap = codecs.lookup('latin1')
#Enc, Dec, rwrap, Wwrap = codecs.lookup('iso8859')
sys.stdin=rwrap(sys.stdin)
sys.stdout=wwrap(sys.stdout)
#sys.stderr=wwrap(sys.stderr)

def emit(line):
  sys.stdout.write(line)

def page_ref(page):
  if page[-1]==",": # then
    page=page[:-1]
    pg=page
    etc=","
  else:
    pg=page
    etc=""
  fi
  pg=re_dot.sub("_",pg)
  return '<a HREF="#%(pg)s">%(page)s</a>%(etc)s'%vars()
end(page_ref)

def add_href(line):
  out=""
  is_page=False
  for tok in re_page.split(line): # do
    if is_page: # then
      out+=page_ref(tok)
    else:
      out+=tok
    fi
    is_page=not is_page 
  od
  return out
end(add_href)

def emit_name(line):
  page=re_newpage.match(line).groupdict()["page"]
  emit('<a name="%(page)s"></a>'%vars())
end(emit_name)

lineno=0
done=[]

for file in sys.argv[1:]: # do
  prev_chapt=False
  prev_side=False # on left
  prev_pre=False;
  in_pre=False
  prev_p=False
  in_p=False
  is_cont=False
  in_index=False
  on_right=False
  in_table=True
  for line in rwrap(open(file)): # do
    lineno+=1
    line=line.rstrip()
    if re_ignore.match(line): # then
      emit(line.join(("<!-- "," \n-->")))
      continue
    fi
    if re_newpage.match(line): # then
      emit_name(line)
      #emit('<center><a name="%(page)s">%(page)s</a></center>'%vars())
      emit(line.join(("<!-- "," \n-->")))
      continue
    fi
    if not line: continue
    if lineno<=5: # then
      print line.join(("<CENTER><H%d>"%lineno,"</H%d></CENTER>"%lineno))
      continue
    fi
    while re_index.search(line) and len(line)>max_stop: # do
      line=re_index.sub(".  .",line,1)
    od
    while re_index.search(line) and len(line)<max_stop: # do
      line=re.sub("[.]  [.]"," .  .",line,1)
    od
    if ".  ." in line: # then
      line=add_href(line)
    fi
    if re_chapt.match(line): # then
      id=re_ws.sub("_",line)
      print line.join(('<HR><H2 ID="%(id)s"><CENTER>'%vars(),'</CENTER></H2>'))
      prev_chapt=True
      continue
    fi
    if prev_chapt: # then
      id=re_ws.sub("_",line)
      print line.join(('<H2 ID="%(id)s"><CENTER>'%vars(),'</CENTER></H2>'))
      prev_chapt=False
      continue
    fi
    if re_special.match(line): # then
      print line.strip().join(("<H3><CENTER>","</CENTER></H3>"))
      continue
    fi
    if line=="Index":
      in_index=True
      print line.join(("<H2><CENTER>","</CENTER></H2>"))
      print '<TABLE border="2" valign=top width="90%"><TR><TD valign=top><pre>'
      continue
    fi
    if in_index: # then
      on_right=bool(re_right.match(line))
      if on_right and not prev_side: emit("</pre></td><td valign=top><pre>") 
      if not on_right and prev_side: emit("</pre></td></tr><tr><td valign=top><pre>")
      if on_right: line=re_right.sub("",line)
      line=add_href(line)
      print line
      prev_side=on_right
      continue
    fi
    
    #line=re.sub("2The","The",line) # special case
    #line=re.sub("3When","When",line) # special case
    line=re.sub(u'Kl¨oke',u"Klöke",line) # special case
    line=re.sub("\s[1-9][0-9]*([A-Z][a-z]+)",r"\g<1>",line) # special case
  
    if is_cont: line=line.lstrip()
    line=re_copy.sub("&copy;",line)
    line=re_bull.sub("&bull;",line)
    
  
    if not is_cont: # then
      in_pre=in_p=a_heading=False
      # classify line
      if re_heading.match(line) and not re_index.search(line) and not re_ex.match(line): # then:
        a_heading=True
      elif re_ex.match(line): # then
        in_pre=True
      elif re_p.match(line) and not re_ex.match(line): # then
        in_p=True
      else:
        in_pre=line[0]==" " or re_index.search(line)
        if not in_pre:
          in_p=True
        fi
      fi
  
  #  print "<!--",sorted([ (k,v) for k,v in vars().items() if k[0]!="_" and isinstance(v,bool)]),"-->"
      if not in_pre and prev_pre: emit("</pre>")
      if not in_p and in_p: emit("</p>")
    fi
    line=re.sub(u"æ", "&aelig;",line)
    if a_heading: # then
      id=re_dot.sub("_",re.sub("\s.*","",line))
      # Todo: make id2 more unique
      id2=re_dot.sub("_",re.sub("^[^\s]*\s*","",line))
      if id2 in done: # then
        print line.join(('<H3 id="%(id)s">'%vars(),'</H3>'))
      else:
        print line.join(('<H3 id="%(id)s" id="%(id2)s">'%vars(),'</H3>'))
        done.append(id2) # XXX
      fi
    else:
      if in_pre and not prev_pre:
        emit("<pre>\n")
        if line.startswith("     ") and not re_a_prefix.search(line): 
          line=line[2:] # XXX
        fi
      fi
      if in_p and not prev_p: 
        emit("<p>")
      fi
      if re_hr.search(line): # then
        if not prev_pre and not in_table:
          line=re_hr.sub('<table  border="2" width="90%"><tr><td>',line)
          in_table=True
        else:
          line=re_hr.sub("<td><tr></table>",line)
          in_table=False
        fi 
      #line=re_hr.sub("<hr>",line)
      if not in_pre and re_cont.search(line): # then
        sys.stdout.write(line[:-1])
        is_cont=True
      else: 
        line=re_a_prefix.sub(to_a_prefix,line)
        print line
        if increase3 in line: to_a_prefix = to_a_prefix2
        is_cont=False
      fi
      if in_p and re_stop.search(line) and len(line)<=max_stop: # then
        in_p=False
      fi
    fi
    prev_pre=in_pre 
    prev_p=in_p 
  od
  print "</pre></td></tr></TABLE>"
od

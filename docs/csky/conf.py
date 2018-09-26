
# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#

# -- Project information

project = u'C-SKY QEMU user guide'
author = u'C-SKY'
copyright = 'copyright'

# -- General configuration

extensions = [
]
templates_path = ['_templates']
source_suffix = '.rst'
master_doc = 'index'
language = u'zh_CN'
exclude_patterns = []
pygments_style = 'sphinx'
numfig = 1

# -- Options for HTML output

html_theme = 'alabaster'
html_static_path = ['_static']

# -- Options for HTMLHelp output

htmlhelp_basename = 'cksim-doc'

# -- Options for LaTeX output

latex_engine = 'xelatex'

latex_elements = {
	'preamble': r'''
\usepackage[UTF8, heading = true]{ctex}
\setlength{\parindent}{2em}
\hypersetup{bookmarksnumbered = true}
\setcounter{secnumdepth}{3}
\usepackage{graphicx}
\makeatletter
    \fancypagestyle{normal}{
        \fancyhf{}
        \fancyfoot[R]{\thepage}
        \fancyfoot[L]{\py@release}
        \fancyfoot[C]{\fontsize{7}{7} \selectfont Copyright \textcopyright\ 2018 Hangzhou C-SKY MicroSystems Co.,Ltd. All rights reserved.}
        \fancyhead[L]{\includegraphics[scale=0.4] {cskylog.png}}
        \fancyhead[R]{\py@HeaderFamily \nouppercase{\leftmark}}
     }
\makeatother
	''',
	'classoptions': ',oneside',
	'maketitle': ur'''
\maketitle

\small \textbf{Copyright © 2018 杭州中天微系统有限公司，保留所有权利。}

本文档的产权属于杭州中天微系统有限公司(下称中天公司)。本文档仅能分布给:(i)拥有合法雇佣关系，
并需要本文档的信息的中天微系统员工，或(ii)非中天微组织但拥有合法合作关系，
并且其需要本文档的信息的合作方。对于本文档，禁止任何在专利、版权或商业秘密过程中，
授予或暗示的可以使用该文档。在没有得到杭州中天微系统有限公司的书面许可前，
不得复制本文档的任何部分，传播、转录、储存在检索系统中或翻译成任何语言或计算机语言。

\textbf{商标申明}

杭州中天微系统的LOGO和其它所有商标归杭州中天微系统有限公司所有，所有其它产品或服务名称归其所有者拥有。

\textbf{注意}

您购买的产品、服务或特性等应受中天公司商业合同和条款的约束，
本文档中描述的全部或部分产品、服务或特性可能不在您的购买或使用范围之内。
除非合同另有约定，中天公司对本文档内容不做任何明示或默示的声明或保证。
由于产品版本升级或其他原因，本文档内容会不定期进行更新。
除非另有约定，本文档仅作为使用指导，本文档中的所有陈述、信息和建议不构成任何明示或暗示的担保。

\vspace{10pt}

\textbf{Copyright © 2018 Hangzhou C-SKY MicroSystems Co.,Ltd. All rights reserved.}

This document is the property of Hangzhou C-SKY MicroSystems Co.,Ltd. 
This document may only be distributed to: (i) a C-SKY party having a 
legitimate business need for the information contained herein, 
or (ii) a non-C-SKY party having a legitimate business need for the 
information contained herein.  No license, expressed or implied, 
under any patent, copyright or trade secret right is granted or 
implied by the conveyance of this document. No part of this document 
may be reproduced, transmitted, transcribed, stored in a retrieval system, 
translated into any language or computer language, in any form or by any means, 
electronic, mechanical, magnetic, optical, chemical, manual, or otherwise 
without the prior written permission of C-SKY MicroSystems Co.,Ltd.

\textbf{Trademarks and Permissions}

The C-SKY Logo and all other trademarks indicated as such herein 
are trademarks of Hangzhou C-SKY MicroSystems Co.,Ltd. All other 
products or service names are the property of their respective owners.

\textbf{Notice}

The purchased products, services and features are stipulated by 
the contract made between C-SKY and the customer. All or part of the 
products, services and features described in this document may not be 
within the purchase scope or the usage scope. Unless otherwise specified 
in the contract, all statements, information, and recommendations in 
this document are provided "AS IS" without warranties, guarantees or 
representations of any kind, either express or implied.
The information in this document is subject to change without notice. 
Every effort has been made in the preparation of this document 
to ensure accuracy of the contents, but all statements, 
information, and recommendations in this document do not constitute 
a warranty of any kind, express or implied.

\vspace{30pt}

杭州中天微系统有限公司 C-SKY MicroSystems Co.,LTD

地址:  杭州市西湖区西斗门路3号天堂软件园A座15楼

邮编: 310012

网址:  www.c-sky.com
	''',

     'passoptionstopackages': r'\PassOptionsToPackage{svgnames}{xcolor}',
     'sphinxsetup': 'verbatimwithframe=false, VerbatimColor={rgb}{0.968,0.968,0.968}, TitleColor={named}{DodgerBlue}',
}

latex_additional_files = ["_static/cskylog.png"]

latex_documents = [
    (master_doc, u'cksim.tex', u'C-SKY QEMU 手册',
     u'C-SKY', 'manual'),
]

# -- Options for Texinfo output

texinfo_documents = [
    (master_doc, 'cksim', u'cksim Documentation',
     author, 'cksim', 'One line description of project.',
     'Miscellaneous'),
]

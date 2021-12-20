# Project Title
自然场景渲染

# Team member

| 学号    | 姓名   |
| ------- | ------ |
| 1950483 | 謝子康 |
| 1950638 | 陈冠忠 |
| 1951042 | 王远洋 |
| 1951125 | 农烨   |
| 1952217 | 孙泽凯 |
| 1952704 | 王麒斌 |

# Motivation

计算机图形学在众多领域有广泛的应用。其中，室外场景渲染是图形学重要的研究课题，也是3D图形引擎的核心。本小组紧扣课程所教内容，尝试充分运用渲染知识，针对一个较为复杂的场景，进行真实美观的渲染。

场景的选择中，自然场景较社会场景具有普遍性，因此成为本组的项目目标。自动场景纷繁复杂，水体、光照、云、树木等物体颇多。本组尝试对其部分景物实现建模与渲染。

# The Goal of the project

1. 导入复杂的露天场景，包含水面、多光源、物理天空；
2. 实现PBR渲染流程，实现水面的屏幕空间反射；
3. 尝试采用多种程序方式增强画面表现力，包括HDR、Bloom、SSAO等；
4. 尝试采用多种美术方式增强画面表现力，如烘焙高精度PBR贴图；
5. 阴影效果的渲染尝试基于方差阴影(Variance Shadow Map)的软阴影。

# The Scope of the project

1. 基于Ray-Marching的体积渲染，包含体积光、体积云；
2. 基于粒子系统的环境，包含火焰、雪等；
3. 增加更多增强画面表现力的方式，如景深、镜头光晕；
4. 阴影实现部分尝试使用基于Moment Shadow Mapping的软阴影。

# Involved CG techniques


1. 延迟渲染 + SSAO
    为了实现更好的模型加载效果，尤其是对于模型褶皱、孔洞和非常靠近的墙面变暗的方法近似模拟出间接光照，引入了屏幕空间环境光遮蔽（SSAO）技术。由于SSAO与延迟渲染完美兼容，因此，在延迟着色的基础之上引入SSAO技术。通过G缓冲获取几何体的信息，通过法向半球以及随机核心转动的采样计算SSAO，并且添加环境遮蔽模糊为呈现更好的视觉效果，以此实现SSAO效果。

2. 体积光
    模拟自然环境，体积光自然是必不可少的效果。自然场景中的烟、雾、光、云等产生的效果可以通过图形学的方式来实现。体积光的实现采用着色器绘制的方式，即在着色器中计算体积光的光路效果。本次使用的是Ray-Match的方式来实现体积光，通过采样步长来计算效果。为了模拟太阳光这样的光源的效果，加入大气散射的效果，即Mie散射，计算的公式如下：
    $$
    f_{HG}(\theta)=\frac{(1-g)^2}{4\pi (1+g^2-2g\cdot cos(\theta))^{3/2}}
    $$

    实现的效果如下：
    <img src="https://raw.githubusercontent.com/yuanyangwangTJ/Picture/master/img/20211219224552.png" alt="image-20211219224544727" style="zoom:67%;" />

# Implementation

# Result

# References
[1] [learnopengl](https://learnopengl.com/)
[2] [volume light rendering](https://stackoverflow.com/questions/65784612/volumetric-light-not-rendering-volume-correctly)
[3] [体积光效果实现](https://blog.csdn.net/ZJU_fish1996/article/details/87533029)

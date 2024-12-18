T.assert(GG.rootPass() != null, "root pass is not null");

ScreenPass pass0, pass1;

T.assert(pass0.next() == null, "default next is NULL");
pass0 --> pass1;
T.assert(pass0.next() == pass1, "next is pass1");
T.assert(pass1.next() == null, "pass1 next is NULL");
pass0 --< pass1;
T.assert(pass0.next() == null, "ungruck pass is null");


RenderPass rpass;
T.assert(rpass.colorOutput() == null, "default target is null");
Texture target;
rpass.colorOutput(target);
T.assert(rpass.colorOutput() == target, "target is target");
rpass.colorOutput(null);
T.assert(rpass.colorOutput() == null, "target is null again");

T.assert(rpass.autoClearColor(), "default auto clear color is true");
rpass.autoClearColor(false);
T.assert(!rpass.autoClearColor(), "auto clear color is false");

T.assert(GG.renderPass().next() == GG.hudPass(), "default render pass next is hud pass");
T.assert(GG.hudPass().next() == GG.outputPass(), "default hud pass next is output pass");
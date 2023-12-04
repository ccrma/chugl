#include "ulib_gui.h"
#include "ulib_cgl.h"

#include "renderer/scenegraph/SceneGraphObject.h"
#include "renderer/scenegraph/Locator.h"
#include "renderer/scenegraph/Mesh.h"
#include "renderer/scenegraph/Material.h"
#include "renderer/scenegraph/Geometry.h"

using namespace GUI;

// GUI Static Initialization ==============================================================

//-----------------------------------------------------------------------------
// name: Manager 
//-----------------------------------------------------------------------------
std::vector<Window*> Manager::s_Windows;
std::mutex Manager::s_WindowsLock;
CBufferSimple* Manager::s_SharedEventQueue;
const Chuck_DL_Api* Manager::s_CKAPI;
Chuck_VM* Manager::s_CKVM;
Manager::CkTypeMap Manager::s_CkTypeMap = {
    {Type::Element, "UI_Element"},
    {Type::Window, "UI_Window"},
    {Type::Button, "UI_Button"},
    {Type::FloatSliderBase, "UI_SliderFloatBase"},
    {Type::FloatSlider, "UI_SliderFloat"},
    {Type::FloatSlider2, "UI_SliderFloat2"},
    {Type::FloatSlider3, "UI_SliderFloat3"},
    {Type::FloatSlider4, "UI_SliderFloat4"},
    {Type::IntSliderBase, "UI_SliderIntBase"},
    {Type::IntSlider, "UI_SliderInt"},
    {Type::IntSlider2, "UI_SliderInt2"},
    {Type::IntSlider3, "UI_SliderInt3"},
    {Type::IntSlider4, "UI_SliderInt4"},
    {Type::Checkbox, "UI_Checkbox"},
    {Type::Color3, "UI_Color3"},
    {Type::Dropdown, "UI_Dropdown"},
    {Type::Text, "UI_Text"},
    {Type::GGenTree, "UI_GGen"},
    {Type::InputText, "UI_InputText"}
};

void Manager::DrawGUI()
{
    // grab the window lock
    std::lock_guard<std::mutex> lock(s_WindowsLock);

    // draw all windows
    for (auto& window : s_Windows)
    {
        window->Draw();
    }
}

void Manager::AddWindow(Window *window)
{
    std::lock_guard<std::mutex> lock(GetWindowLock());
    s_Windows.push_back(window);
}

void GUI::Element::SetLabel(const std::string & label)
{
    // using the shared-window level lock
    // can be written to by chuck audio thread via GUI API
    // will be read every frame by render thread while drawing GUI
    // threadsafe now because render thread grabs this same lock when drawing
    std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
    // dear imgui does NOT support empty string, so we use a space instead
    m_Label = label.empty() ? " " : label;
}

//-----------------------------------------------------------------------------
// name: Text 
//-----------------------------------------------------------------------------
const t_CKUINT Text::DefaultText = Text::Mode::Default;
const t_CKUINT Text::BulletText = Text::Mode::Bullet;
const t_CKUINT Text::SeparatorText = Text::Mode::Separator;



// Chuck API =================================================================

//-----------------------------------------------------------------------------
// Element  (base class for all chugl gui elements)
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_element_ctor );
CK_DLL_DTOR( chugl_gui_element_dtor );
CK_DLL_MFUN( chugl_gui_element_label_set );
CK_DLL_MFUN( chugl_gui_element_label_get );

//-----------------------------------------------------------------------------
// Window
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_window_ctor );
CK_DLL_MFUN( chugl_gui_window_add_element );

//-----------------------------------------------------------------------------
// Button
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_button_ctor );

//-----------------------------------------------------------------------------
// Checkbox 
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_checkbox_ctor );
CK_DLL_MFUN( chugl_gui_checkbox_val_get );
CK_DLL_MFUN( chugl_gui_checkbox_val_set );

//-----------------------------------------------------------------------------
// FloatSliderBase
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_float_base_ctor );
CK_DLL_MFUN( chugl_gui_slider_float_range_set );
CK_DLL_MFUN( chugl_gui_slider_float_range_get );
CK_DLL_MFUN( chugl_gui_slider_float_power_set );
CK_DLL_MFUN( chugl_gui_slider_float_power_get );

//-----------------------------------------------------------------------------
// FloatSliderBase --> FloatSlider1
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_float_ctor );
CK_DLL_MFUN( chugl_gui_slider_float_val_get );
CK_DLL_MFUN( chugl_gui_slider_float_val_set );

//-----------------------------------------------------------------------------
// FloatSliderBase --> FloatSlider2
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_float_2_ctor );
CK_DLL_MFUN( chugl_gui_slider_float_2_val_get );
CK_DLL_MFUN( chugl_gui_slider_float_2_val_set );

//-----------------------------------------------------------------------------
// FloatSliderBase --> FloatSlider3
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_float_3_ctor );
CK_DLL_MFUN( chugl_gui_slider_float_3_val_get );
CK_DLL_MFUN( chugl_gui_slider_float_3_val_set );

//-----------------------------------------------------------------------------
// FloatSliderBase --> FloatSlider4
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_float_4_ctor );
CK_DLL_MFUN( chugl_gui_slider_float_4_val_get );
CK_DLL_MFUN( chugl_gui_slider_float_4_val_set );

//-----------------------------------------------------------------------------
// IntSliderBase
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_int_base_ctor );
CK_DLL_MFUN( chugl_gui_slider_int_range_set );
CK_DLL_MFUN( chugl_gui_slider_int_range_get );

//-----------------------------------------------------------------------------
// IntSliderBase --> IntSlider
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_int_ctor );
CK_DLL_MFUN( chugl_gui_slider_int_val_get );
CK_DLL_MFUN( chugl_gui_slider_int_val_set );

//-----------------------------------------------------------------------------
// IntSliderBase --> IntSlider2
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_int_2_ctor );
CK_DLL_MFUN( chugl_gui_slider_int_2_val_get );
CK_DLL_MFUN( chugl_gui_slider_int_2_val_set );

//-----------------------------------------------------------------------------
// IntSliderBase --> IntSlider3
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_int_3_ctor );
CK_DLL_MFUN( chugl_gui_slider_int_3_val_get );
CK_DLL_MFUN( chugl_gui_slider_int_3_val_set );

//-----------------------------------------------------------------------------
// IntSliderBase --> IntSlider4
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_int_4_ctor );
CK_DLL_MFUN( chugl_gui_slider_int_4_val_get );
CK_DLL_MFUN( chugl_gui_slider_int_4_val_set );



//-----------------------------------------------------------------------------
// Color3
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_color3_ctor );
CK_DLL_MFUN( chugl_gui_color3_val_get );
CK_DLL_MFUN( chugl_gui_color3_val_set );

//-----------------------------------------------------------------------------
// Dropdown 
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_dropdown_ctor );
CK_DLL_MFUN( chugl_gui_dropdown_options_set );
CK_DLL_MFUN( chugl_gui_dropdown_val_get );
CK_DLL_MFUN( chugl_gui_dropdown_val_set );

//-----------------------------------------------------------------------------
// Text
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_text_ctor );
CK_DLL_MFUN( chugl_gui_text_val_get );
CK_DLL_MFUN( chugl_gui_text_val_set );
CK_DLL_MFUN( chugl_gui_text_color3_get );
CK_DLL_MFUN( chugl_gui_text_color3_set );
// CK_DLL_MFUN( chugl_gui_text_color4_get );
// CK_DLL_MFUN( chugl_gui_text_color4_set );
CK_DLL_MFUN( chugl_gui_text_wrap_get );
CK_DLL_MFUN( chugl_gui_text_wrap_set );
CK_DLL_MFUN( chugl_gui_text_mode_get );
CK_DLL_MFUN( chugl_gui_text_mode_set );

t_CKBOOL init_chugl_gui_element(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_window(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_button(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_checkbox(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_slider_float(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_slider_int(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_color3(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_dropdown(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_text(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_input_text(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_ggen_tree(Chuck_DL_Query *QUERY);

//-----------------------------------------------------------------------------
// InputText 
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_input_text_ctor );
CK_DLL_MFUN( chugl_gui_input_text_val_get );
CK_DLL_MFUN( chugl_gui_input_text_val_set );
CK_DLL_MFUN( chugl_gui_input_text_is_multiline_get );
CK_DLL_MFUN( chugl_gui_input_text_is_multiline_set );
CK_DLL_MFUN( chugl_gui_input_text_multiline_size_get );
CK_DLL_MFUN( chugl_gui_input_text_multiline_size_set );
CK_DLL_MFUN( chugl_gui_input_text_broadcast_on_enter_get );
CK_DLL_MFUN( chugl_gui_input_text_broadcast_on_enter_set );

//-----------------------------------------------------------------------------
// GGen Tree 
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_ggen_tree_ctor );
CK_DLL_MFUN( chugl_gui_ggen_tree_root_get );
CK_DLL_MFUN( chugl_gui_ggen_tree_root_set );


t_CKBOOL init_chugl_gui(Chuck_DL_Query *QUERY)
{
    // get the VM and API
    Chuck_VM * vm = QUERY->ck_vm(QUERY);
    CK_DL_API api = QUERY->ck_api(QUERY);

    // initialize Manager static references
    Manager::SetCKAPI(api);
    Manager::SetCKVM(vm);
	Manager::SetEventQueue(
        api->vm->create_event_buffer(vm)
    );

    if (!init_chugl_gui_element(QUERY)) return FALSE;
    if (!init_chugl_gui_window(QUERY)) return FALSE;
    if (!init_chugl_gui_button(QUERY)) return FALSE;
    if (!init_chugl_gui_checkbox(QUERY)) return FALSE;
    if (!init_chugl_gui_slider_float(QUERY)) return FALSE;
    if (!init_chugl_gui_slider_int(QUERY)) return FALSE;
    if (!init_chugl_gui_color3(QUERY)) return FALSE;
    if (!init_chugl_gui_dropdown(QUERY)) return FALSE;
    if (!init_chugl_gui_text(QUERY)) return FALSE;
    if (!init_chugl_gui_input_text(QUERY)) return FALSE;
    if (!init_chugl_gui_ggen_tree(QUERY)) return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// name: init_chugl_gui_element()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_element(Chuck_DL_Query *QUERY)
{   
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Element), "Event");
	QUERY->doc_class(QUERY, 
        "Base class for all GUI elements. Do not instantiate directly"
        "All GUI elements are Chuck Events, and can be used as such."
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_element_ctor);
    QUERY->add_dtor(QUERY, chugl_gui_element_dtor);
    
    // add member
    CGL::SetGUIDataOffset(QUERY->add_mvar(QUERY, "int", "@data", FALSE));

    // add label()
    QUERY->add_mfun(QUERY, chugl_gui_element_label_set, "string", "text");
    QUERY->add_arg( QUERY,"string", "text" );
    QUERY->doc_func(QUERY, "set the text label of this element");

    // add label()
    QUERY->add_mfun(QUERY, chugl_gui_element_label_get, "string", "text");
    QUERY->doc_func(QUERY, "get the text label of this element");

    
    // QUERY->add_mfun(QUERY, mauielement_display, "void", "display");
    
    // add hide()
    // QUERY->add_mfun(QUERY, mauielement_hide, "void", "hide");
    
    // add destroy()
    // QUERY->add_mfun(QUERY, mauielement_destroy, "void", "destroy");
    
    // add name()
    // QUERY->add_mfun(QUERY, mauielement_name_set, "string", "name");
    // QUERY->add_arg( QUERY,"string", "n" );
    
    // add name()
    // QUERY->add_mfun(QUERY, mauielement_name_get, "string", "name");
    
    // add size()
    // QUERY->add_mfun(QUERY, mauielement_size, "void", "size");
    // QUERY->add_arg( QUERY,"float", "w" );
    // QUERY->add_arg( QUERY,"float", "h" );
    
    // add width()
    // QUERY->add_mfun(QUERY, mauielement_width, "float", "width");
    
    // add height()
    // QUERY->add_mfun(QUERY, mauielement_height, "float", "height");
    
    // add position()
    // QUERY->add_mfun(QUERY, mauielement_position, "void", "position");
    // QUERY->add_arg( QUERY,"float", "x" );
    // QUERY->add_arg( QUERY,"float", "y" );
    
    // add x()
    // QUERY->add_mfun(QUERY, mauielement_x, "float", "x");
    
    // add y()
    // QUERY->add_mfun(QUERY, mauielement_y, "float", "y");
    
    // add onChange()
    // QUERY->add_mfun(QUERY, mauielement_onchange, "Event", "onChange");
    
    // end the class import
    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_element_ctor )
{
    // no constructor, let subclasses handle
}

CK_DLL_DTOR( chugl_gui_element_dtor )
{
    GUI::Element* element = (GUI::Element*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = 0;
    // TODO only going to clear chuck mem for now
    // no destructors yet, can do mem management + delete later
}

CK_DLL_MFUN( chugl_gui_element_label_set )
{
    Element* element = (Element*)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    Chuck_String * s = GET_NEXT_STRING(ARGS);
    if( element && s )
    {
        element->SetLabel( API->object->str(s) );
        RETURN->v_string = s;
    }
    else
    {
        RETURN->v_string = NULL;
    }
}

CK_DLL_MFUN( chugl_gui_element_label_get )
{
    Element* element = (Element*)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_string = (Chuck_String*)API->object->create_string(VM, element->GetLabel().c_str(), false);
}

//-----------------------------------------------------------------------------
// name: init_chugl_gui_window()
// desc: ...
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_gui_window( Chuck_DL_Query * QUERY )
{
    // import
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Window), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Window element. Each instance will create a separate GUI window."
        "Add elements to the window via .add() to display them."
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");
   

    QUERY->add_ctor( QUERY, chugl_gui_window_ctor );
    
    QUERY->add_mfun(QUERY, chugl_gui_window_add_element, "void", "add");
    QUERY->add_arg( QUERY, Manager::GetCkName(Type::Element), "element" );
    QUERY->doc_func(QUERY, "Adds an element to this window, will be displayed in the order they are added.");
    
    // wrap up
    QUERY->end_class(QUERY);
    
    return TRUE;
}

CK_DLL_CTOR( chugl_gui_window_ctor )
{
    Window* window = new Window(SELF);
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) window;
    // add window to manager
    Manager::AddWindow(window);
}

CK_DLL_MFUN( chugl_gui_window_add_element )
{
    Window* window = (Window*)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    Element* e = (Element *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), CGL::GetGUIDataOffset());
    window->AddElement(e);
}

//-----------------------------------------------------------------------------
// name: init_chugl_gui_button()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_button(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Button), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, "Button widget, clicking will trigger the button instance, which itself is a Chuck Event");
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_button_ctor);
    // no destructor, let Element handle
    
    // // add get_state()
    // QUERY->add_mfun(QUERY, mauibutton_get_state, "int", "state");
    
    // // add set_state()
    // QUERY->add_mfun(QUERY, mauibutton_set_state, "int", "state");
    // QUERY->add_arg( QUERY,"int", "s" );
    
    // // add pushType()
    // QUERY->add_mfun(QUERY, mauibutton_push_type, "void", "pushType");
    
    // // add toggleType()
    // QUERY->add_mfun(QUERY, mauibutton_toggle_type, "void", "toggleType");
    
    // // add unsetImage()
    // QUERY->add_mfun(QUERY, mauibutton_unset_image, "void", "unsetImage");
    
    // // add setImage()
    // QUERY->add_mfun(QUERY, mauibutton_set_image, "int", "setImage");
    // QUERY->add_arg( QUERY,"string", "filepath" );
    
    // wrap up
    QUERY->end_class(QUERY);
    
    return TRUE;
}

CK_DLL_CTOR( chugl_gui_button_ctor ) {
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new Button(SELF);
}


//-----------------------------------------------------------------------------
// name: init_chugl_gui_checkbox()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_checkbox(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Checkbox), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, "Checkbox widget");
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");
    
    QUERY->add_ctor(QUERY, chugl_gui_checkbox_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_checkbox_val_get, "int", "val");
    QUERY->doc_func(QUERY, "Get the current state of the checkbox, 1 for checked, 0 for unchecked");

    QUERY->add_mfun(QUERY, chugl_gui_checkbox_val_set, "int", "val");
    QUERY->add_arg( QUERY, "int", "val" );
    QUERY->doc_func(QUERY, "Set the current state of the checkbox, 1 for checked, 0 for unchecked");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_checkbox_ctor ) {
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new Checkbox(SELF);
}

CK_DLL_MFUN( chugl_gui_checkbox_val_get )
{
    Checkbox* cb = (Checkbox *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_int = cb->GetData() ? 1 : 0;
}

CK_DLL_MFUN( chugl_gui_checkbox_val_set )
{
    Checkbox* cb = (Checkbox *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKINT val = GET_NEXT_INT(ARGS);
    cb->SetData( val ? true : false );
    RETURN->v_int = val;
}


//-----------------------------------------------------------------------------
// name: init_chugl_gui_slider_float()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_slider_float(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::FloatSliderBase), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Base class for float slider widget"
        "Don't instaniate directly, use one of the subclasses e.g. UI_SliderFloat or UI_SliderFloat2"
        "CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds."
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_slider_float_base_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_range_set, "void", "range");
    QUERY->add_arg( QUERY, "float", "min" );
    QUERY->add_arg( QUERY, "float", "max" );
    QUERY->doc_func(QUERY, "Set the range of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_range_get, "vec2", "range");
    QUERY->doc_func(QUERY, "Get the range of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_power_set, "void", "power");
    QUERY->add_arg( QUERY, "float", "power" );
    QUERY->doc_func(QUERY, "Set the power of the slider, e.g. 2.0 for an exponential domain. Defaults to 1.0");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_power_get, "float", "power");
    QUERY->doc_func(QUERY, "Get the power of the slider");

    QUERY->end_class(QUERY);

    // SliderFloat1 =================================================================
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::FloatSlider), Manager::GetCkName(Type::FloatSliderBase));
	QUERY->doc_class(QUERY, 
        "Float slider widget"
    );

    QUERY->add_ctor(QUERY, chugl_gui_slider_float_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_val_get, "float", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_val_set, "float", "val");
    QUERY->add_arg( QUERY, "float", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->end_class(QUERY);

    // SliderFloat2 =================================================================
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::FloatSlider2), Manager::GetCkName(Type::FloatSliderBase));
    QUERY->doc_class(QUERY, 
        "Float2 slider widget. 2 float sliders side by side"
    );
    QUERY->add_ex(QUERY, "fx/custom-fx.ck");

    QUERY->add_ctor(QUERY, chugl_gui_slider_float_2_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_2_val_get, "vec2", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_2_val_set, "vec2", "val");
    QUERY->add_arg( QUERY, "vec2", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->end_class(QUERY);

    // SliderFloat3 =================================================================
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::FloatSlider3), Manager::GetCkName(Type::FloatSliderBase));
    QUERY->doc_class(QUERY, 
        "Float3 slider widget. 3 float sliders side by side"
    );

    QUERY->add_ctor(QUERY, chugl_gui_slider_float_3_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_3_val_get, "vec3", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_3_val_set, "vec3", "val");
    QUERY->add_arg( QUERY, "vec3", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->end_class(QUERY);

    // SliderFloat4 =================================================================
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::FloatSlider4), Manager::GetCkName(Type::FloatSliderBase));
    QUERY->doc_class(QUERY, 
        "Float4 slider widget. 4 float sliders side by side"
    );

    QUERY->add_ctor(QUERY, chugl_gui_slider_float_4_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_4_val_get, "vec4", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_4_val_set, "vec4", "val");
    QUERY->add_arg( QUERY, "vec4", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_slider_float_base_ctor )
{
    // abstract base class, don't instantiate directly
}

static void FloatSliderSetData(
    unsigned int N,
    Chuck_Object* SELF, void* ARGS, Chuck_DL_Return* RETURN, Chuck_VM* VM, Chuck_VM_Shred* SHRED, CK_DL_API API
)
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    if (N == 1) {
        t_CKFLOAT data = GET_NEXT_FLOAT(ARGS);
        slider->SetData({data});
        RETURN->v_float = data;
    } else if (N == 2) {
        t_CKVEC2 vec = GET_NEXT_VEC2(ARGS);
        slider->SetData({vec.x, vec.y});
        RETURN->v_vec2 = vec;
    } else if (N == 3) {
        t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
        slider->SetData({vec.x, vec.y, vec.z});
        RETURN->v_vec3 = vec;
    } else if (N == 4) {
        t_CKVEC4 vec = GET_NEXT_VEC4(ARGS);
        slider->SetData({vec.x, vec.y, vec.z, vec.w});
        RETURN->v_vec4 = vec;
    }
}

static void FloatSliderGetData(
    unsigned int N,
    Chuck_Object* SELF, void* ARGS, Chuck_DL_Return* RETURN, Chuck_VM* VM, Chuck_VM_Shred* SHRED, CK_DL_API API
) {
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    auto& data = slider->GetData();
    if (N == 1) {
        RETURN->v_float = data[0];
    } else if (N == 2) {
        RETURN->v_vec2 = {data[0], data[1]};
    } else if (N == 3) {
        RETURN->v_vec3 = {data[0], data[1], data[2]};
    } else if (N == 4) {
        RETURN->v_vec4 = {data[0], data[1], data[2], data[3]};
    }
}

CK_DLL_MFUN( chugl_gui_slider_float_range_set )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    slider->SetMin( GET_NEXT_FLOAT(ARGS) ); 
    slider->SetMax( GET_NEXT_FLOAT(ARGS) ); 
}

CK_DLL_MFUN( chugl_gui_slider_float_power_set )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    slider->SetPower( GET_NEXT_FLOAT(ARGS) ); 
}

CK_DLL_MFUN( chugl_gui_slider_float_range_get )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_vec2 = {slider->GetMin(), slider->GetMax()};
}\

CK_DLL_MFUN( chugl_gui_slider_float_power_get )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_float = slider->GetPower();
}

// FloatSlider1 =================================================================

CK_DLL_CTOR( chugl_gui_slider_float_ctor ) { OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new FloatSlider(SELF, 1); }
CK_DLL_MFUN( chugl_gui_slider_float_val_get ) { FloatSliderGetData(1, SELF, ARGS, RETURN, VM, SHRED, API); }
CK_DLL_MFUN( chugl_gui_slider_float_val_set ) { FloatSliderSetData(1, SELF, ARGS, RETURN, VM, SHRED, API); }


// FloatSlider2 =================================================================

CK_DLL_CTOR( chugl_gui_slider_float_2_ctor ) { OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new FloatSlider(SELF, 2); }
CK_DLL_MFUN( chugl_gui_slider_float_2_val_get ) { FloatSliderGetData(2, SELF, ARGS, RETURN, VM, SHRED, API); }
CK_DLL_MFUN( chugl_gui_slider_float_2_val_set ) { FloatSliderSetData(2, SELF, ARGS, RETURN, VM, SHRED, API); }

// FloatSlider3 =================================================================

CK_DLL_CTOR( chugl_gui_slider_float_3_ctor ) { OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new FloatSlider(SELF, 3); }
CK_DLL_MFUN( chugl_gui_slider_float_3_val_get ) { FloatSliderGetData(3, SELF, ARGS, RETURN, VM, SHRED, API); }
CK_DLL_MFUN( chugl_gui_slider_float_3_val_set ) { FloatSliderSetData(3, SELF, ARGS, RETURN, VM, SHRED, API); }

// FloatSlider4 =================================================================

CK_DLL_CTOR( chugl_gui_slider_float_4_ctor ) { OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new FloatSlider(SELF, 4); }
CK_DLL_MFUN( chugl_gui_slider_float_4_val_get ) { FloatSliderGetData(4, SELF, ARGS, RETURN, VM, SHRED, API); }
CK_DLL_MFUN( chugl_gui_slider_float_4_val_set ) { FloatSliderSetData(4, SELF, ARGS, RETURN, VM, SHRED, API); }


//-----------------------------------------------------------------------------
// name: init_chugl_gui_slider_int()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_slider_int(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::IntSliderBase), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Int slider Base class widget"
        "CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds."
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_slider_int_base_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_range_set, "void", "range");
    QUERY->add_arg( QUERY, "int", "min" );
    QUERY->add_arg( QUERY, "int", "max" );
    QUERY->doc_func(QUERY, "Set the range of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_range_get, "vec2", "range");
    QUERY->doc_func(QUERY, "Get the range of the slider");

    QUERY->end_class(QUERY);

    // IntSlider1 =================================================================
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::IntSlider), Manager::GetCkName(Type::IntSliderBase));
    QUERY->doc_class(QUERY, 
        "Int slider widget"
        "CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds."
    );

    QUERY->add_ctor(QUERY, chugl_gui_slider_int_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_val_get, "int", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_val_set, "int", "val");
    QUERY->add_arg( QUERY, "int", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->end_class(QUERY);

    // IntSlider2 =================================================================
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::IntSlider2), Manager::GetCkName(Type::IntSliderBase));
    QUERY->doc_class(QUERY, 
        "Int2 slider widget. 2 int sliders side by side"
    );

    QUERY->add_ctor(QUERY, chugl_gui_slider_int_2_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_2_val_get, "vec2", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_2_val_set, "vec2", "val");
    QUERY->add_arg( QUERY, "vec2", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->end_class(QUERY);

    // IntSlider3 =================================================================
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::IntSlider3), Manager::GetCkName(Type::IntSliderBase));
    QUERY->doc_class(QUERY, 
        "Int3 slider widget. 3 int sliders side by side"
    );

    QUERY->add_ctor(QUERY, chugl_gui_slider_int_3_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_3_val_get, "vec3", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_3_val_set, "vec3", "val");
    QUERY->add_arg( QUERY, "vec3", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->end_class(QUERY);

    // IntSlider4 =================================================================
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::IntSlider4), Manager::GetCkName(Type::IntSliderBase));
    QUERY->doc_class(QUERY, 
        "Int4 slider widget. 4 int sliders side by side"
    );

    QUERY->add_ctor(QUERY, chugl_gui_slider_int_4_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_4_val_get, "vec4", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_4_val_set, "vec4", "val");
    QUERY->add_arg( QUERY, "vec4", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_slider_int_base_ctor )
{
    // abstract base class, don't instantiate directly
}

CK_DLL_MFUN( chugl_gui_slider_int_range_set )
{
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    slider->SetMin( GET_NEXT_INT(ARGS) ); 
    slider->SetMax( GET_NEXT_INT(ARGS) ); 
}

CK_DLL_MFUN( chugl_gui_slider_int_range_get )
{
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_vec2 = {1.0 * slider->GetMin(), 1.0 * slider->GetMax()};
}

static void IntSliderSetData(
    unsigned int N,
    Chuck_Object* SELF, void* ARGS, Chuck_DL_Return* RETURN, Chuck_VM* VM, Chuck_VM_Shred* SHRED, CK_DL_API API
)
{
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    if (N == 1) {
        t_CKINT data = GET_NEXT_INT(ARGS);
        slider->SetData({1.0 * data});
        RETURN->v_int = data;
    } else if (N == 2) {
        t_CKVEC2 vec = GET_NEXT_VEC2(ARGS);
        slider->SetData({vec.x, vec.y});
        RETURN->v_vec2 = vec;
    } else if (N == 3) {
        t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
        slider->SetData({vec.x, vec.y, vec.z});
        RETURN->v_vec3 = vec;
    } else if (N == 4) {
        t_CKVEC4 vec = GET_NEXT_VEC4(ARGS);
        slider->SetData({vec.x, vec.y, vec.z, vec.w});
        RETURN->v_vec4 = vec;
    }
}

static void IntSliderGetData(
    unsigned int N,
    Chuck_Object* SELF, void* ARGS, Chuck_DL_Return* RETURN, Chuck_VM* VM, Chuck_VM_Shred* SHRED, CK_DL_API API
) {
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    auto& data = slider->GetData();
    if (N == 1) {
        RETURN->v_int = data[0];
    } else if (N == 2) {
        RETURN->v_vec2 = {1.0 * data[0], 1.0 * data[1]};
    } else if (N == 3) {
        RETURN->v_vec3 = {1.0 * data[0], 1.0 * data[1], 1.0 * data[2]};
    } else if (N == 4) {
        RETURN->v_vec4 = { 1.0 * data[0], 1.0 * data[1], 1.0 * data[2], 1.0 * data[3] };
    }
}

// IntSlider1 =================================================================
CK_DLL_CTOR( chugl_gui_slider_int_ctor ) { OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new IntSlider(SELF, 1); }
CK_DLL_MFUN( chugl_gui_slider_int_val_get ) { IntSliderGetData(1, SELF, ARGS, RETURN, VM, SHRED, API); }
CK_DLL_MFUN( chugl_gui_slider_int_val_set ) { IntSliderSetData(1, SELF, ARGS, RETURN, VM, SHRED, API); }

// IntSlider2 =================================================================
CK_DLL_CTOR( chugl_gui_slider_int_2_ctor ) { OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new IntSlider(SELF, 2); }
CK_DLL_MFUN( chugl_gui_slider_int_2_val_get ) { IntSliderGetData(2, SELF, ARGS, RETURN, VM, SHRED, API); }
CK_DLL_MFUN( chugl_gui_slider_int_2_val_set ) { IntSliderSetData(2, SELF, ARGS, RETURN, VM, SHRED, API); }

// IntSlider3 =================================================================
CK_DLL_CTOR( chugl_gui_slider_int_3_ctor ) { OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new IntSlider(SELF, 3); }
CK_DLL_MFUN( chugl_gui_slider_int_3_val_get ) { IntSliderGetData(3, SELF, ARGS, RETURN, VM, SHRED, API); }
CK_DLL_MFUN( chugl_gui_slider_int_3_val_set ) { IntSliderSetData(3, SELF, ARGS, RETURN, VM, SHRED, API); }

// IntSlider4 =================================================================
CK_DLL_CTOR( chugl_gui_slider_int_4_ctor ) { OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new IntSlider(SELF, 4); }
CK_DLL_MFUN( chugl_gui_slider_int_4_val_get ) { IntSliderGetData(4, SELF, ARGS, RETURN, VM, SHRED, API); }
CK_DLL_MFUN( chugl_gui_slider_int_4_val_set ) { IntSliderSetData(4, SELF, ARGS, RETURN, VM, SHRED, API); }


//-----------------------------------------------------------------------------
// name: init_chugl_gui_color3()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_color3(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Color3), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Color picker widget"
        "Ttip: the ColorEdit* functions have a little colored preview square that can be left-clicked to open a picker, and right-clicked to open an option menu."
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_color3_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_color3_val_get, "vec3", "val");
    QUERY->doc_func(QUERY, "Get the current value of the color picker");

    QUERY->add_mfun(QUERY, chugl_gui_color3_val_set, "vec3", "val");
    QUERY->add_arg( QUERY, "vec3", "color");
    QUERY->doc_func(QUERY, "Set the current value of the color picker");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_color3_ctor )
{
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new Color3(SELF);
}

CK_DLL_MFUN( chugl_gui_color3_val_get )
{
    Color3* color3 = (Color3 *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    glm::vec3 color = color3->GetData();
    RETURN->v_vec3 = {color.r, color.g, color.b};
}

CK_DLL_MFUN( chugl_gui_color3_val_set )
{
    Color3* color3 = (Color3 *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
    color3->SetData( color );
    RETURN->v_vec3 = color;
}

//-----------------------------------------------------------------------------
// name: init_chugl_gui_dropdown()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_dropdown(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Dropdown), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Dropdown widget"
        "Create a dropdown menu with the given list of items"
        "On select, the widget will store the selected items *index* and will trigger an event"
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_dropdown_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_dropdown_val_get, "int", "val");
    QUERY->doc_func(QUERY, "Get the integer index of the curerntly selected dropdown item");

    QUERY->add_mfun(QUERY, chugl_gui_dropdown_val_set, "int", "val");
    QUERY->add_arg( QUERY, "int", "idx");
    QUERY->doc_func(QUERY, "Set the integer index of the selected dropdown item");


    QUERY->add_mfun(QUERY, chugl_gui_dropdown_options_set, "void", "options");
    QUERY->add_arg( QUERY, "string[]", "options");
    QUERY->doc_func(QUERY, "Set the list of options for the dropdown menu");


    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_dropdown_ctor )
{
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new Dropdown(SELF);
}

CK_DLL_MFUN( chugl_gui_dropdown_val_get )
{
    Dropdown* dropdown = (Dropdown *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_int = dropdown->GetData();
}

CK_DLL_MFUN( chugl_gui_dropdown_val_set )
{
    Dropdown* dropdown = (Dropdown *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKINT idx = GET_NEXT_INT(ARGS);
    dropdown->SetData( idx );
    RETURN->v_int = idx;
}

CK_DLL_MFUN( chugl_gui_dropdown_options_set )
{
    Dropdown* dropdown = (Dropdown *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    Chuck_ArrayInt * options = (Chuck_ArrayInt *) GET_NEXT_OBJECT(ARGS);
    
    // TODO: extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_int_size(options);
    std::vector<std::string> options_str;
    options_str.reserve(vsize);
    // pull chuck_strings out of the array
    for( t_CKINT idx = 0; idx < vsize; idx++ )
    {
        // cast to chuck string *
        Chuck_String * ckstr = (Chuck_String *)API->object->array_int_get_idx(options,idx);
        // convert to c++ string and put into array
        options_str.push_back( API->object->str(ckstr) );
    }

    dropdown->SetOptions( options_str );
}


//-----------------------------------------------------------------------------
// name: init_chugl_gui_text()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_text(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Text), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Text widget"
        "Add text to the UI window"
        "Supports options such as color, style, and wrapping"
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_svar(QUERY, "int", "MODE_DEFAULT", true, (void *)&Text::DefaultText);
    QUERY->doc_var(QUERY, "default text mode, no additional formatting");

    QUERY->add_svar(QUERY, "int", "MODE_BULLET", true, (void *)&Text::BulletText);
    QUERY->doc_var(QUERY, "display text as a bullet point");

    QUERY->add_svar(QUERY, "int", "MODE_SEPARATOR", true, (void *)&Text::SeparatorText);
    QUERY->doc_var(QUERY, "display text with a horizontal separator line");

    QUERY->add_ctor(QUERY, chugl_gui_text_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_text_val_get, "string", "text");
    QUERY->doc_func(QUERY, "Set the text string");

    QUERY->add_mfun(QUERY, chugl_gui_text_val_set, "string", "text");
    QUERY->add_arg( QUERY, "string", "text");
    QUERY->doc_func(QUERY, "Get the text string");

    QUERY->add_mfun(QUERY, chugl_gui_text_color3_get, "vec3", "color");
    QUERY->doc_func(QUERY, "Get the text color");

    QUERY->add_mfun(QUERY, chugl_gui_text_color3_set, "vec3", "color");
    QUERY->add_arg( QUERY, "vec3", "color");
    QUERY->doc_func(QUERY, "Set the text color, defaults to alpha of 1.0 ie fully opaque");

    // will enable when can create a vec4 type in chuck from a vec3 like:
    // @(Color.RED, 1.0)

    // QUERY->add_mfun(QUERY, chugl_gui_text_color4_get, "vec4", "color");
    // QUERY->doc_func(QUERY, "Get the text color");

    // QUERY->add_mfun(QUERY, chugl_gui_text_color4_set, "vec4", "color");
    // QUERY->add_arg( QUERY, "vec4", "color");
    // QUERY->doc_func(QUERY, "Set the text color");

    QUERY->add_mfun(QUERY, chugl_gui_text_wrap_get, "int", "wrap");
    QUERY->doc_func(QUERY, "Get the text wrapping mode. Default True, meaning text will wrap to end of window");

    QUERY->add_mfun(QUERY, chugl_gui_text_wrap_set, "int", "wrap");
    QUERY->add_arg( QUERY, "int", "wrap");
    QUERY->doc_func(QUERY, 
        "Set the text wrapping mode. Default True, meaning text will wrap to end of window"
        "If false, text will extend past the window bounds, and the user will need to scroll horizontally to see it"
    );

    QUERY->add_mfun(QUERY, chugl_gui_text_mode_get, "int", "mode");
    QUERY->doc_func(QUERY, 
        "Get the text mode. Default is MODE_DEFAULT, meaning no additional formatting"
        "MODE_BULLET will display text as a bullet point"
        "MODE_SEPARATOR will display text with a horizontal separator line"
    );

    QUERY->add_mfun(QUERY, chugl_gui_text_mode_set, "int", "mode");
    QUERY->add_arg( QUERY, "int", "mode");
    QUERY->doc_func(QUERY, 
        "Set the text mode. Default is MODE_DEFAULT, meaning no additional formatting"
        "MODE_BULLET will display text as a bullet point"
        "MODE_SEPARATOR will display text with a horizontal separator line"
    );

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_text_ctor )
{
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new Text(SELF);
}

CK_DLL_MFUN( chugl_gui_text_val_get )
{
    Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_string = (Chuck_String*)API->object->create_string(VM, text->GetData().c_str(), false);
}

CK_DLL_MFUN( chugl_gui_text_val_set )
{
    Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    Chuck_String * s = GET_NEXT_STRING(ARGS);
    if( text && s )
    {
        text->SetData( API->object->str(s) );
        RETURN->v_string = s;
    }
    else
    {
        RETURN->v_string = NULL;
    }
}

CK_DLL_MFUN( chugl_gui_text_color3_get )
{
    Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    glm::vec4 color = text->GetColor();
    RETURN->v_vec3 = {color.r, color.g, color.b};
}

CK_DLL_MFUN( chugl_gui_text_color3_set )
{
    Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
    text->SetColor( {color.x, color.y, color.z, 1.0 } );
    RETURN->v_vec3 = color;
}

// CK_DLL_MFUN( chugl_gui_text_color4_get )
// {
//     Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
//     glm::vec4 color = text->GetColor();
//     RETURN->v_vec4 = {color.r, color.g, color.b, color.a};
// }

// CK_DLL_MFUN( chugl_gui_text_color4_set )
// {
//     Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
//     t_CKVEC4 color = GET_NEXT_VEC4(ARGS);
//     text->SetColor( { color.x, color.y, color.z, color.w } );
//     RETURN->v_vec4 = color;
// }

CK_DLL_MFUN( chugl_gui_text_wrap_get )
{
    Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_int = text->GetWrap() ? 1 : 0;
}

CK_DLL_MFUN( chugl_gui_text_wrap_set )
{
    Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKINT wrap = GET_NEXT_INT(ARGS);
    text->SetWrap( wrap ? true : false );
    RETURN->v_int = wrap;
}

CK_DLL_MFUN( chugl_gui_text_mode_get )
{
    Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_int = text->GetMode();
}

CK_DLL_MFUN( chugl_gui_text_mode_set )
{
    Text* text = (Text*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKINT mode = GET_NEXT_INT(ARGS);
    text->SetMode( static_cast<Text::Mode>(mode) );
    RETURN->v_int = mode;
}

//-----------------------------------------------------------------------------
// name: init_chugl_gui_input_text()
// desc: Text input field
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_input_text(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::InputText), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Input Text widget"
        "Get user text input"
        "Supports options such as multi-line mode, and whether to broadcast on every edit or only on <enter>"
    );
    QUERY->add_ex(QUERY, "basic/gtext.ck");

    QUERY->add_ctor(QUERY, chugl_gui_input_text_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_input_text_val_get, "string", "input");
    QUERY->doc_func(QUERY, "Get the current input to the text box");

    QUERY->add_mfun(QUERY, chugl_gui_input_text_val_set, "string", "input");
    QUERY->add_arg( QUERY, "string", "text");
    QUERY->doc_func(QUERY, "Set the current input to the text box");

    QUERY->add_mfun(QUERY, chugl_gui_input_text_is_multiline_get, "int", "multiline");
    QUERY->doc_func(QUERY, "Get whether the text box is in multi-line mode");

    QUERY->add_mfun(QUERY, chugl_gui_input_text_is_multiline_set, "int", "multiline");
    QUERY->add_arg( QUERY, "int", "multiline");
    QUERY->doc_func(QUERY, 
        "If true, sets the input field to multi-line mode. Else sets it to single-line mode"
        "In multi-line mode you can set the size of the input box with .size()"
        "In multi-line mode, <enter> will insert a newline, while <ctrl+enter> will broadcast this UI event if broadcastOnEnter() is set"
    );

    QUERY->add_mfun(QUERY, chugl_gui_input_text_multiline_size_get, "vec2", "size");
    QUERY->doc_func(QUERY, "Get the size of the input box in multi-line mode");

    QUERY->add_mfun(QUERY, chugl_gui_input_text_multiline_size_set, "vec2", "size");
    QUERY->add_arg( QUERY, "vec2", "size");
    QUERY->doc_func(QUERY, 
        "Set the size of the input box in multi-line mode."
        "Values are in the units of font height in pixels"
        "Setting the x-dimension <= 0 will set the width to the window width"
        "The y-dimension defaults to 16 * fontHeight"
        "Settings the y-dmension <= 0 will set the height to the window height"
    );

    QUERY->add_mfun(QUERY, chugl_gui_input_text_broadcast_on_enter_get, "int", "broadcastOnEnter");
    QUERY->doc_func(QUERY, "Get whether the text box broadcasts on every edit or only on <enter>");

    QUERY->add_mfun(QUERY, chugl_gui_input_text_broadcast_on_enter_set, "int", "broadcastOnEnter");
    QUERY->add_arg( QUERY, "int", "broadcastOnEnter");
    QUERY->doc_func(QUERY, 
        "If true, sets the input field to broadcast on every edit. Else sets it to broadcast only on <enter>"
        "In multi-line mode, <enter> will insert a newline, while <ctrl+enter> will broadcast this UI event if broadcastOnEnter() is set"
    );

    QUERY->end_class(QUERY);

    return TRUE;
}


CK_DLL_CTOR( chugl_gui_input_text_ctor )
{
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new InputText(SELF);
}
CK_DLL_MFUN( chugl_gui_input_text_val_get )
{
    InputText* input_text = (InputText*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_string = (Chuck_String*)API->object->create_string(VM, input_text->GetData().c_str(), false);
}

CK_DLL_MFUN( chugl_gui_input_text_val_set )
{
    InputText* input_text = (InputText*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    Chuck_String * s = GET_NEXT_STRING(ARGS);
    input_text->SetData( API->object->str(s) );
    RETURN->v_string = s;
}

CK_DLL_MFUN( chugl_gui_input_text_is_multiline_get )
{
    InputText* input_text = (InputText*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_int = input_text->GetMultiLine() ? 1 : 0;
}

CK_DLL_MFUN( chugl_gui_input_text_is_multiline_set )
{
    InputText* input_text = (InputText*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKINT multiline = GET_NEXT_INT(ARGS);
    input_text->SetMultiLine( multiline ? true : false );
    RETURN->v_int = multiline;
}

CK_DLL_MFUN( chugl_gui_input_text_multiline_size_get )
{
    InputText* input_text = (InputText*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    glm::vec2 size = input_text->GetMultiLineInputSize();
    RETURN->v_vec2 = {size.x, size.y};
}

CK_DLL_MFUN( chugl_gui_input_text_multiline_size_set )
{
    InputText* input_text = (InputText*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKVEC2 size = GET_NEXT_VEC2(ARGS);
    input_text->SetMultiLineInputSize( {size.x, size.y} );
    RETURN->v_vec2 = size;
}

CK_DLL_MFUN( chugl_gui_input_text_broadcast_on_enter_get )
{
    InputText* input_text = (InputText*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_int = input_text->GetBroadcastOnEnter() ? 1 : 0;
}

CK_DLL_MFUN( chugl_gui_input_text_broadcast_on_enter_set )
{
    InputText* input_text = (InputText*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKINT broadcast_on_enter = GET_NEXT_INT(ARGS);
    input_text->SetBroadcastOnEnter( broadcast_on_enter ? true : false );
    RETURN->v_int = broadcast_on_enter;
}


//-----------------------------------------------------------------------------
// name: init_chugl_gui_ggen_tree()
// desc: Simple GGen Tree view (as one would get in an Editor scenegraph)
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_gui_ggen_tree(Chuck_DL_Query* QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::GGenTree), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "GGen tree widget"
        "View data about a given GGen and all its children"
        "A simple version of a typical Editor Scenegraph"
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_ggen_tree_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_ggen_tree_root_get, "GGen", "root");
    QUERY->doc_func(QUERY, "Get the root GGen of the tree. Will start here when drawing the scene tree");

    QUERY->add_mfun(QUERY, chugl_gui_ggen_tree_root_set, "GGen", "root");
    QUERY->add_arg( QUERY, "GGen", "root");
    QUERY->doc_func(QUERY, "Set the root GGen of the tree. Will start here when drawing the scene tree");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_ggen_tree_ctor )
{
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new GGenTree(SELF);
}

CK_DLL_MFUN( chugl_gui_ggen_tree_root_get )
{
    GGenTree* ggen_tree = (GGenTree*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    auto* node = Locator::GetNode(ggen_tree->m_RootID, true);
    RETURN->v_object = node ? node->m_ChuckObject : NULL;
}

CK_DLL_MFUN( chugl_gui_ggen_tree_root_set )
{
    GGenTree* ggen_tree = (GGenTree*) OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    Chuck_Object* ckobj = GET_NEXT_OBJECT(ARGS);
    if (ckobj) {
        ggen_tree->m_RootID = CGL::GetSGO(ckobj)->GetID();
    }
    RETURN->v_object = ckobj;
}

void GUI::GGenTree::Draw()
{       
    if (!ImGui::CollapsingHeader(m_Label.c_str())) return;
    DrawImpl(dynamic_cast<SceneGraphObject*>(Locator::GetNode(m_RootID, false)));
}

void GUI::GGenTree::DrawImpl(SceneGraphObject* node)
{       
    if (!node) return;
    // note: TreeNode(...) needs a unique string label for each node to disambiguate clicks
    std::string label = "[" + std::string(node->myCkName()) + " " + std::to_string(node->GetID()) + "] " + node->GetName();
    if (ImGui::TreeNode(label.c_str())) {
        // transform
        ImGui::SeparatorText("Transform");

        glm::vec3 pos = node->GetPosition();
        ImGui::DragFloat3("Position", &pos[0], 0.0f);

        glm::vec3 rot = node->GetEulerRotationRadians();
        ImGui::DragFloat3("Rotation", &rot[0], 0.0f);

        glm::vec3 scale = node->GetScale();
        ImGui::DragFloat3("Scale", &scale[0], 0.0f);

        if (node->IsMesh()) { DrawMesh(dynamic_cast<Mesh*>(node)); }

        // children
        int numChildren = node->GetChildren().size();
        std::string childrenText = std::string("Children: ") + std::to_string(numChildren);
        ImGui::SeparatorText(childrenText.c_str());
        for (auto* child : node->GetChildren()) {
            DrawImpl(child);
        }

        // pop tree
        ImGui::TreePop();
    }
}

void GUI::GGenTree::DrawMesh(Mesh *node)
{
    if (!node) return;

    ImGui::SeparatorText("Mesh");

    // material info
    Material* material = node->GetMaterial();
    std::string materialLabel = "Material: [" + std::string(material->myCkName()) + " " + std::to_string(material->GetID()) + "] " + material->GetName();
    ImGui::Text(materialLabel.c_str());

    // geometry info
    Geometry* geometry = node->GetGeometry();
    std::string geometryLabel = "Geometry: [" + std::string(geometry->myCkName()) + " " + std::to_string(geometry->GetID()) + "] " + geometry->GetName();
    ImGui::Text(geometryLabel.c_str());
}

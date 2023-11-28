#include "ulib_gui.h"
#include "ulib_cgl.h"

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
    {Type::FloatSlider, "UI_SliderFloat"},
    {Type::IntSlider, "UI_SliderInt"},
    {Type::Checkbox, "UI_Checkbox"},
    {Type::Color3, "UI_Color3"},
    {Type::Dropdown, "UI_Dropdown"},
    {Type::Text, "UI_Text"}
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
// FloatSlider
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_float_ctor );
CK_DLL_MFUN( chugl_gui_slider_float_val_get );
CK_DLL_MFUN( chugl_gui_slider_float_val_set );
CK_DLL_MFUN( chugl_gui_slider_float_range_set );
CK_DLL_MFUN( chugl_gui_slider_float_power_set );

//-----------------------------------------------------------------------------
// IntSlider
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_int_ctor );
CK_DLL_MFUN( chugl_gui_slider_int_val_get );
CK_DLL_MFUN( chugl_gui_slider_int_val_set );
CK_DLL_MFUN( chugl_gui_slider_int_range_set );

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
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::FloatSlider), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Float slider widget"
        "CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds."
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_slider_float_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_val_get, "float", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_val_set, "float", "val");
    QUERY->add_arg( QUERY, "float", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_range_set, "void", "range");
    QUERY->add_arg( QUERY, "float", "min" );
    QUERY->add_arg( QUERY, "float", "max" );
    QUERY->doc_func(QUERY, "Set the range of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_power_set, "void", "power");
    QUERY->add_arg( QUERY, "float", "power" );
    QUERY->doc_func(QUERY, "Set the power of the slider, e.g. 2.0 for an exponential domain. Defaults to 1.0");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_slider_float_ctor )
{
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new FloatSlider(SELF);
}

CK_DLL_MFUN( chugl_gui_slider_float_val_get )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_float = slider->GetData();
}

CK_DLL_MFUN( chugl_gui_slider_float_val_set )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKFLOAT val = GET_NEXT_FLOAT(ARGS);
    slider->SetData( val );
    RETURN->v_float = val;
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


//-----------------------------------------------------------------------------
// name: init_chugl_gui_slider_int()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_slider_int(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::IntSlider), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, 
        "Int slider widget"
        "CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds."
    );
    QUERY->add_ex(QUERY, "ui/basic-ui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_slider_int_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_val_get, "int", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_val_set, "int", "val");
    QUERY->add_arg( QUERY, "int", "val" );
    QUERY->doc_func(QUERY, "Set the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_range_set, "void", "range");
    QUERY->add_arg( QUERY, "int", "min" );
    QUERY->add_arg( QUERY, "int", "max" );
    QUERY->doc_func(QUERY, "Set the range of the slider");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_slider_int_ctor )
{
    OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset()) = (t_CKINT) new IntSlider(SELF);
}

CK_DLL_MFUN( chugl_gui_slider_int_val_get )
{
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    RETURN->v_int= slider->GetData();
}

CK_DLL_MFUN( chugl_gui_slider_int_val_set )
{
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    t_CKINT val = GET_NEXT_INT(ARGS);
    slider->SetData( val );
    RETURN->v_int = val;
}

CK_DLL_MFUN( chugl_gui_slider_int_range_set )
{
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, CGL::GetGUIDataOffset());
    slider->SetMin( GET_NEXT_INT(ARGS) ); 
    slider->SetMax( GET_NEXT_INT(ARGS) ); 
}

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

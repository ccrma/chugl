#include "ulib_gui.h"

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
    {Type::Element, "GUI_Element"},
    {Type::Window, "GUI_Window"},
    {Type::Button, "GUI_Button"},
    {Type::FloatSlider, "GUI_FloatSlider"},
    {Type::IntSlider, "GUI_IntSlider"},
    {Type::Checkbox, "GUI_Checkbox"},
    {Type::Color3, "GUI_Color3"}
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

//-----------------------------------------------------------------------------
// FloatSlider
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_float_ctor );
CK_DLL_MFUN( chugl_gui_slider_float_val_get );
CK_DLL_MFUN( chugl_gui_slider_float_range_set );
CK_DLL_MFUN( chugl_gui_slider_float_power_set );

//-----------------------------------------------------------------------------
// IntSlider
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_slider_int_ctor );
CK_DLL_MFUN( chugl_gui_slider_int_val_get );
CK_DLL_MFUN( chugl_gui_slider_int_range_set );

//-----------------------------------------------------------------------------
// Color3
//-----------------------------------------------------------------------------
CK_DLL_CTOR( chugl_gui_color3_ctor );
CK_DLL_MFUN( chugl_gui_color3_val_get );


t_CKBOOL init_chugl_gui_element(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_window(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_button(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_checkbox(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_slider_float(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_slider_int(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_gui_color3(Chuck_DL_Query *QUERY);


t_CKBOOL init_chugl_gui(Chuck_DL_Query *QUERY)
{   
    // initialize Manager static references
    Manager::SetCKAPI( QUERY->api() );
    Manager::SetCKVM( QUERY->vm() );
	Manager::SetEventQueue(
        QUERY->api()->vm->create_event_buffer(QUERY->vm())
    );

    if (!init_chugl_gui_element(QUERY)) return FALSE;
    if (!init_chugl_gui_window(QUERY)) return FALSE;
    if (!init_chugl_gui_button(QUERY)) return FALSE;
    if (!init_chugl_gui_checkbox(QUERY)) return FALSE;
    if (!init_chugl_gui_slider_float(QUERY)) return FALSE;
    if (!init_chugl_gui_slider_int(QUERY)) return FALSE;
    if (!init_chugl_gui_color3(QUERY)) return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// name: init_chugl_gui_element()
// desc: ...
//-----------------------------------------------------------------------------
static t_CKINT chugl_gui_element_offset_data = 0;
t_CKBOOL init_chugl_gui_element(Chuck_DL_Query *QUERY)
{   
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Element), "Event");
	QUERY->doc_class(QUERY, 
        "Base class for all GUI elements. Do not instantiate directly"
        "All GUI elements are Chuck Events, and can be used as such."
    );
    QUERY->add_ex(QUERY, "gui/basic-gui.ck");

    QUERY->add_ctor(QUERY, chugl_gui_element_ctor);
    QUERY->add_dtor(QUERY, chugl_gui_element_dtor);
    
    // add member
    chugl_gui_element_offset_data = QUERY->add_mvar(QUERY, "int", "@data", FALSE);

    // add label()
    QUERY->add_mfun(QUERY, chugl_gui_element_label_set, "string", "label");
    QUERY->add_arg( QUERY,"string", "l" );
    QUERY->doc_func(QUERY, "set the text label of this element");

    // add label()
    QUERY->add_mfun(QUERY, chugl_gui_element_label_get, "string", "label");
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
    GUI::Element* element = (GUI::Element*) OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
    OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data) = 0;
    // TODO only going to clear chuck mem for now
    // no destructors yet, can do mem management + delete later
}

CK_DLL_MFUN( chugl_gui_element_label_set )
{
    Element* element = (Element*)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
    Chuck_String * s = GET_NEXT_STRING(ARGS);
    if( element && s )
    {
        element->SetLabel( s->str() );
        RETURN->v_string = s;
    }
    else
    {
        RETURN->v_string = NULL;
    }
}

CK_DLL_MFUN( chugl_gui_element_label_get )
{
    Element* element = (Element*)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
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
    OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data) = (t_CKINT) window;
    // add window to manager
    Manager::AddWindow(window);
}

CK_DLL_MFUN( chugl_gui_window_add_element )
{
    Window* window = (Window*)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
    Element* e = (Element *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), chugl_gui_element_offset_data);
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
    OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data) = (t_CKINT) new Button(SELF);
}


//-----------------------------------------------------------------------------
// name: init_chugl_gui_checkbox()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_gui_checkbox(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Manager::GetCkName(Type::Checkbox), Manager::GetCkName(Type::Element));
	QUERY->doc_class(QUERY, "Checkbox widget");
    
    QUERY->add_ctor(QUERY, chugl_gui_checkbox_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_checkbox_val_get, "int", "val");
    QUERY->doc_func(QUERY, "Get the current state of the checkbox, 1 for checked, 0 for unchecked");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_checkbox_ctor ) {
    OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data) = (t_CKINT) new Checkbox(SELF);
}

CK_DLL_MFUN( chugl_gui_checkbox_val_get ) {
    Checkbox* cb = (Checkbox *)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
    RETURN->v_int = cb->GetData() ? 1 : 0;
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

    QUERY->add_ctor(QUERY, chugl_gui_slider_float_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_val_get, "float", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_range_set, "void", "range");
    QUERY->add_arg( QUERY, "float", "min" );
    QUERY->add_arg( QUERY, "float", "max" );
    QUERY->doc_func(QUERY, "Set the range of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_float_power_set, "void", "power");
    QUERY->add_arg( QUERY, "float", "power" );
    QUERY->doc_func(QUERY, "Set the power of the slider, e.g. 2.0 for an exponential domain");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_slider_float_ctor )
{
    OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data) = (t_CKINT) new FloatSlider(SELF);
}

CK_DLL_MFUN( chugl_gui_slider_float_val_get )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
    RETURN->v_float = slider->GetData();
}

CK_DLL_MFUN( chugl_gui_slider_float_range_set )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
    slider->SetMin( GET_NEXT_FLOAT(ARGS) ); 
    slider->SetMax( GET_NEXT_FLOAT(ARGS) ); 
}

CK_DLL_MFUN( chugl_gui_slider_float_power_set )
{
    FloatSlider* slider = (FloatSlider *)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
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

    QUERY->add_ctor(QUERY, chugl_gui_slider_int_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_val_get, "int", "val");
    QUERY->doc_func(QUERY, "Get the current value of the slider");

    QUERY->add_mfun(QUERY, chugl_gui_slider_int_range_set, "void", "range");
    QUERY->add_arg( QUERY, "int", "min" );
    QUERY->add_arg( QUERY, "int", "max" );
    QUERY->doc_func(QUERY, "Set the range of the slider");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_slider_int_ctor )
{
    OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data) = (t_CKINT) new IntSlider(SELF);
}

CK_DLL_MFUN( chugl_gui_slider_int_val_get )
{
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
    RETURN->v_int= slider->GetData();
}

CK_DLL_MFUN( chugl_gui_slider_int_range_set )
{
    IntSlider* slider = (IntSlider *)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
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

    QUERY->add_ctor(QUERY, chugl_gui_color3_ctor);

    QUERY->add_mfun(QUERY, chugl_gui_color3_val_get, "vec3", "val");
    QUERY->doc_func(QUERY, "Get the current value of the color picker");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR( chugl_gui_color3_ctor )
{
    OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data) = (t_CKINT) new Color3(SELF);
}

CK_DLL_MFUN( chugl_gui_color3_val_get )
{
    Color3* color3 = (Color3 *)OBJ_MEMBER_INT(SELF, chugl_gui_element_offset_data);
    glm::vec3 color = color3->GetData();
    RETURN->v_vec3 = {color.r, color.g, color.b};
}

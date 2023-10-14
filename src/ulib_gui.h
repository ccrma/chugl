// #pragma once

// #include "chuck_def.h"
// #include "chuck_dl.h"


// t_CKBOOL init_chugl_gui(Chuck_DL_Query *QUERY);


// // GUI classes ====================================

// namespace GUI 
// {


// //-----------------------------------------------------------------------------
// // name: Type
// // desc: Type enums for GUI elements
// //-----------------------------------------------------------------------------
// enum Type : unsigned int {
//     Element = 0,
//     Button,
//     Slider,
//     Checkbox

// };


// //-----------------------------------------------------------------------------
// // name: Element
// // desc: abstract GUI element base 
// //-----------------------------------------------------------------------------
// class Element
// {
// public:
//     virtual ~Element();
//     virtual void Draw() = 0;



// private:
//     std::string label;
//     void* m_ReadData;   // read by chuck thread, written to by render thread on widget update.
//                         // not threadsafe, requires lock to read/write

//     void* m_WriteData;  // only ever written to by the Render thread, threadsafe

//     std::mutex m_ReadDataLock;  // lock to provide read/write access to m_ReadData
// };

// class Window : public Element
// {
// public:

// private:
//     bool m_Open;
// }



// }


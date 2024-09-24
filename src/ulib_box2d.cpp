#include <box2d/box2d.h>
#include <chuck/chugin.h>

#include "ulib_helper.h"

/*
Experiment to integrate Box2D without OOP.
This is true to the original Box2D API, and has better performance.
Box2D worlds/bodies/shapes are identified by integer ids stored as t_CKINT.
These integers are used directly, rather than wrapping in a class, like
b2_World. So we avoid a cache miss from dereferencing the chuck object pointer.
This also simplifies the implementation; don't need to worry about
constructors/destructors/refcounting.

Downsides:
- slightly more verbose. world.createBody(body_def) --> B2.createBody(world_id,
body_def)
- no type safety. Everything is an int, rather than b2_World, b2_Body etc.
    - this will be resolved if chuck adds typedef
*/

// make sure we can fit b2 ids within a t_CKINT
static_assert(sizeof(void*) == sizeof(t_CKUINT), "pointer size mismatch");
static_assert(sizeof(b2WorldId) <= sizeof(t_CKINT), "b2Worldsize mismatch");
static_assert(sizeof(b2BodyId) <= sizeof(t_CKINT), "b2Body size mismatch");
static_assert(sizeof(b2ShapeId) <= sizeof(t_CKINT), "b2Shape size mismatch");

// accessors
// custom accessor because size of the b2Id may be less than the corresponding ckobj
// member field (e.g. 32 byte b2WorldId is stored in the slot of a 64byte t_CKINT)
// TODO today: refactor GET_B2_ID out and replace
#define GET_NEXT_B2_ID(type, args, val)                                                \
    *val = (*(type*)args);                                                             \
    ASSERT(sizeof(type <= sizeof(t_CKINT)));                                           \
    GET_NEXT_INT(args) // advance the pointer by amount allocate

#define GET_B2_ID(type, ptr) (*(type*)ptr)
#define RETURN_B2_ID(type, id) *((type*)&(RETURN->v_int)) = (id)

#define OBJ_MEMBER_B2_ID(type, ckobj, offset) (*(type*)OBJ_MEMBER_DATA(ckobj, offset))

#define OBJ_MEMBER_B2_PTR(type, ckobj, offset) (*(type**)OBJ_MEMBER_DATA(ckobj, offset))

#define B2_ID_TO_CKINT(id) (*(t_CKINT*)&(id))

b2BodyType ckint_to_b2BodyType(t_CKINT type)
{
    switch (type) {
        case 0: return b2_staticBody;
        case 1: return b2_kinematicBody;
        case 2: return b2_dynamicBody;
        default: return b2_staticBody;
    }
}

// ckobj data offsets --------------------------------------------
// b2_WorldDef
static t_CKUINT b2_WorldDef_gravity_offset                = 0;
static t_CKUINT b2_WorldDef_restitutionThreshold_offset   = 0;
static t_CKUINT b2_WorldDef_contactPushoutVelocity_offset = 0;
static t_CKUINT b2_WorldDef_hitEventThreshold_offset      = 0;
static t_CKUINT b2_WorldDef_contactHertz_offset           = 0;
static t_CKUINT b2_WorldDef_contactDampingRatio_offset    = 0;
static t_CKUINT b2_WorldDef_jointHertz_offset             = 0;
static t_CKUINT b2_WorldDef_jointDampingRatio_offset      = 0;
static t_CKUINT b2_WorldDef_enableSleep_offset            = 0;
static t_CKUINT b2_WorldDef_enableContinous_offset        = 0;
static t_CKUINT b2_WorldDef_workerCount_offset            = 0;
CK_DLL_CTOR(b2_WorldDef_ctor);

static void ckobj_to_b2WorldDef(CK_DL_API API, b2WorldDef* obj, Chuck_Object* ckobj);
// static void b2WorldDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
//                                 b2WorldDef* obj);

// b2_BodyMoveEvent
static t_CKUINT b2_BodyMoveEvent_pos_offset        = 0;
static t_CKUINT b2_BodyMoveEvent_rot_offset        = 0;
static t_CKUINT b2_BodyMoveEvent_bodyId_offset     = 0;
static t_CKUINT b2_BodyMoveEvent_fellAsleep_offset = 0;

static Arena b2_body_move_event_pool;
static void b2BodyMoveEvent_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                     b2BodyMoveEvent* obj);

// b2_ContactHitEvent
static t_CKUINT b2_ContactHitEvent_shapeIdA_offset      = 0;
static t_CKUINT b2_ContactHitEvent_shapeIdB_offset      = 0;
static t_CKUINT b2_ContactHitEvent_point_offset         = 0;
static t_CKUINT b2_ContactHitEvent_normal_offset        = 0;
static t_CKUINT b2_ContactHitEvent_approachSpeed_offset = 0;

static Arena b2_contact_hit_event_pool; // used so we don't have to malloc a ton of
                                        // contact hit event ckobjs every frame
static void b2ContactHitEvent_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                       b2ContactHitEvent* obj);
// static void ckobj_to_b2ContactHitEvent(CK_DL_API  API, b2ContactHitEvent*
// obj, Chuck_Object* ckobj);

// b2_Filter
static t_CKUINT b2_Filter_categoryBits_offset = 0;
static t_CKUINT b2_Filter_maskBits_offset     = 0;
static t_CKUINT b2_Filter_groupIndex_offset   = 0;
CK_DLL_CTOR(b2_Filter_ctor);

static void b2Filter_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Filter* obj);
static void ckobj_to_b2Filter(CK_DL_API API, b2Filter* obj, Chuck_Object* ckobj);

// b2_ShapeDef
static t_CKUINT b2_ShapeDef_friction_offset             = 0;
static t_CKUINT b2_ShapeDef_restitution_offset          = 0;
static t_CKUINT b2_ShapeDef_density_offset              = 0;
static t_CKUINT b2_ShapeDef_filter_offset               = 0;
static t_CKUINT b2_ShapeDef_isSensor_offset             = 0;
static t_CKUINT b2_ShapeDef_enableSensorEvents_offset   = 0;
static t_CKUINT b2_ShapeDef_enableContactEvents_offset  = 0;
static t_CKUINT b2_ShapeDef_enableHitEvents_offset      = 0;
static t_CKUINT b2_ShapeDef_enablePreSolveEvents_offset = 0;
static t_CKUINT b2_ShapeDef_forceContactCreation_offset = 0;
CK_DLL_CTOR(b2_ShapeDef_ctor);

static void b2ShapeDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2ShapeDef* obj);
static void ckobj_to_b2ShapeDef(CK_DL_API API, b2ShapeDef* obj, Chuck_Object* ckobj);

// b2_Polygon
static t_CKUINT b2_polygon_data_offset = 0;
static Chuck_Object* b2_polygon_create(Chuck_VM_Shred* shred, b2Polygon* polygon)
{
    CK_DL_API API             = g_chuglAPI;
    b2Polygon* poly           = new b2Polygon(*polygon);
    Chuck_Object* polygon_obj = chugin_createCkObj("b2_Polygon", false, shred);
    OBJ_MEMBER_UINT(polygon_obj, b2_polygon_data_offset) = (t_CKUINT)poly;
    return polygon_obj;
}

// b2_BodyDef
static t_CKUINT b2_BodyDef_type_offset            = 0;
static t_CKUINT b2_BodyDef_position_offset        = 0;
static t_CKUINT b2_BodyDef_angle_offset           = 0;
static t_CKUINT b2_BodyDef_linearVelocity_offset  = 0;
static t_CKUINT b2_BodyDef_angularVelocity_offset = 0;
static t_CKUINT b2_BodyDef_linearDamping_offset   = 0;
static t_CKUINT b2_BodyDef_angularDamping_offset  = 0;
static t_CKUINT b2_BodyDef_gravityScale_offset    = 0;
static t_CKUINT b2_BodyDef_sleepThreshold_offset  = 0;
static t_CKUINT b2_BodyDef_enableSleep_offset     = 0;
static t_CKUINT b2_BodyDef_isAwake_offset         = 0;
static t_CKUINT b2_BodyDef_fixedRotation_offset   = 0;
static t_CKUINT b2_BodyDef_isBullet_offset        = 0;
static t_CKUINT b2_BodyDef_isEnabled_offset       = 0;
static t_CKUINT b2_BodyDef_automaticMass_offset   = 0;
CK_DLL_CTOR(b2_BodyDef_ctor);

static void b2BodyDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2BodyDef* obj);
static void ckobj_to_b2BodyDef(CK_DL_API API, b2BodyDef* obj, Chuck_Object* ckobj);

// b2_CastOutput
static t_CKUINT b2_CastOutput_normal_offset     = 0;
static t_CKUINT b2_CastOutput_point_offset      = 0;
static t_CKUINT b2_CastOutput_fraction_offset   = 0;
static t_CKUINT b2_CastOutput_iterations_offset = 0;
static t_CKUINT b2_CastOutput_hit_offset        = 0;

static void b2CastOutput_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                  b2CastOutput* obj);
// static void ckobj_to_b2CastOutput(CK_DL_API API, b2CastOutput* obj,
// Chuck_Object* ckobj);

// API ----------------------------------------------------------------

// b2
CK_DLL_SFUN(chugl_set_b2_world);
CK_DLL_SFUN(b2_set_substep_count);

CK_DLL_SFUN(b2_CreateWorld);
CK_DLL_SFUN(b2_DestroyWorld);

CK_DLL_SFUN(b2_CreateBody);
CK_DLL_SFUN(b2_DestroyBody);

// b2_world
CK_DLL_SFUN(b2_World_IsValid);
CK_DLL_SFUN(b2_World_GetBodyEvents);
CK_DLL_SFUN(b2_World_GetSensorEvents);
CK_DLL_SFUN(b2_World_GetContactEvents);

// b2_Polygon
CK_DLL_DTOR(b2_polygon_dtor);
CK_DLL_SFUN(b2_polygon_make_box);

// b2_Circle
static t_CKUINT b2_Circle_position_offset = 0;
static t_CKUINT b2_Circle_radius_offset   = 0;
CK_DLL_CTOR(b2_Circle_ctor);

static void b2Circle_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Circle* obj);
static void ckobj_to_b2Circle(CK_DL_API API, b2Circle* obj, Chuck_Object* ckobj);

// b2_Capsule
static t_CKUINT b2_Capsule_center1_offset = 0;
static t_CKUINT b2_Capsule_center2_offset = 0;
static t_CKUINT b2_Capsule_radius_offset  = 0;
CK_DLL_CTOR(b2_Capsule_ctor);

static void b2Capsule_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Capsule* obj);
static void ckobj_to_b2Capsule(CK_DL_API API, b2Capsule* obj, Chuck_Object* ckobj);

// b2_Segment
static t_CKUINT b2_Segment_point1_offset = 0;
static t_CKUINT b2_Segment_point2_offset = 0;
CK_DLL_CTOR(b2_Segment_ctor);

static void b2Segment_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Segment* obj);
static void ckobj_to_b2Segment(CK_DL_API API, b2Segment* obj, Chuck_Object* ckobj);

// b2_Shape
// shape creation
CK_DLL_SFUN(b2_CreateCircleShape);
CK_DLL_SFUN(b2_CreateSegmentShape);
CK_DLL_SFUN(b2_CreateCapsuleShape);
CK_DLL_SFUN(b2_CreatePolygonShape);

// shape accessors
CK_DLL_SFUN(b2_Shape_IsValid);
CK_DLL_SFUN(b2_Shape_GetType);
CK_DLL_SFUN(b2_Shape_GetBody);
CK_DLL_SFUN(b2_Shape_IsSensor);
CK_DLL_SFUN(b2_Shape_SetDensity);
CK_DLL_SFUN(b2_Shape_GetDensity);
CK_DLL_SFUN(b2_Shape_SetFriction);
CK_DLL_SFUN(b2_Shape_GetFriction);
CK_DLL_SFUN(b2_Shape_SetRestitution);
CK_DLL_SFUN(b2_Shape_GetRestitution);
CK_DLL_SFUN(b2_Shape_GetFilter);
CK_DLL_SFUN(b2_Shape_SetFilter);
CK_DLL_SFUN(b2_Shape_EnableSensorEvents);
CK_DLL_SFUN(b2_Shape_AreSensorEventsEnabled);
CK_DLL_SFUN(b2_Shape_EnableContactEvents);
CK_DLL_SFUN(b2_Shape_AreContactEventsEnabled);
CK_DLL_SFUN(b2_Shape_EnablePreSolveEvents);
CK_DLL_SFUN(b2_Shape_ArePreSolveEventsEnabled);
CK_DLL_SFUN(b2_Shape_EnableHitEvents);
CK_DLL_SFUN(b2_Shape_AreHitEventsEnabled);
CK_DLL_SFUN(b2_Shape_TestPoint);
CK_DLL_SFUN(b2_Shape_RayCast);
CK_DLL_SFUN(b2_Shape_GetCircle);
CK_DLL_SFUN(b2_Shape_GetSegment);
// CK_DLL_SFUN(b2_Shape_GetSmoothSegment);
CK_DLL_SFUN(b2_Shape_GetCapsule);
CK_DLL_SFUN(b2_Shape_GetPolygon);
CK_DLL_SFUN(b2_Shape_SetCircle);
CK_DLL_SFUN(b2_Shape_SetCapsule);
CK_DLL_SFUN(b2_Shape_SetSegment);
CK_DLL_SFUN(b2_Shape_SetPolygon);
CK_DLL_SFUN(b2_Shape_GetParentChain);
CK_DLL_SFUN(b2_Shape_GetContactCapacity);
// CK_DLL_SFUN(b2_Shape_GetContactData);
CK_DLL_SFUN(b2_Shape_GetAABB);
CK_DLL_SFUN(b2_Shape_GetClosestPoint);

// b2_Body
CK_DLL_SFUN(b2_body_is_valid);
CK_DLL_SFUN(b2_body_get_position);
CK_DLL_SFUN(b2_body_set_type);
CK_DLL_SFUN(b2_body_get_type);
CK_DLL_SFUN(b2_body_get_rotation);
CK_DLL_SFUN(b2_body_get_angle);
CK_DLL_SFUN(b2_body_set_transform);
CK_DLL_SFUN(b2_body_get_local_point);
CK_DLL_SFUN(b2_body_get_world_point);
CK_DLL_SFUN(b2_body_get_local_vector);
CK_DLL_SFUN(b2_body_get_world_vector);
CK_DLL_SFUN(b2_body_get_linear_velocity);
CK_DLL_SFUN(b2_body_set_linear_velocity);
CK_DLL_SFUN(b2_body_get_angular_velocity);
CK_DLL_SFUN(b2_body_set_angular_velocity);
CK_DLL_SFUN(b2_body_apply_force);
CK_DLL_SFUN(b2_body_apply_force_to_center);
CK_DLL_SFUN(b2_body_apply_torque);
CK_DLL_SFUN(b2_body_apply_linear_impulse);
CK_DLL_SFUN(b2_body_apply_linear_impulse_to_center);
CK_DLL_SFUN(b2_body_apply_angular_impulse);
CK_DLL_SFUN(b2_body_get_mass);
CK_DLL_SFUN(b2_body_get_inertia);
CK_DLL_SFUN(b2_body_get_local_center_of_mass);
CK_DLL_SFUN(b2_body_get_world_center_of_mass);
CK_DLL_SFUN(b2_body_apply_mass_from_shapes);
CK_DLL_SFUN(b2_body_set_linear_damping);
CK_DLL_SFUN(b2_body_get_linear_damping);
CK_DLL_SFUN(b2_body_set_angular_damping);
CK_DLL_SFUN(b2_body_get_angular_damping);
CK_DLL_SFUN(b2_body_set_gravity_scale);
CK_DLL_SFUN(b2_body_get_gravity_scale);
CK_DLL_SFUN(b2_body_is_awake);
CK_DLL_SFUN(b2_body_set_awake);
CK_DLL_SFUN(b2_body_enable_sleep);
CK_DLL_SFUN(b2_body_is_sleep_enabled);
CK_DLL_SFUN(b2_body_set_sleep_threshold);
CK_DLL_SFUN(b2_body_get_sleep_threshold);
CK_DLL_SFUN(b2_body_is_enabled);
CK_DLL_SFUN(b2_body_disable);
CK_DLL_SFUN(b2_body_enable);
CK_DLL_SFUN(b2_body_set_fixed_rotation);
CK_DLL_SFUN(b2_body_is_fixed_rotation);
CK_DLL_SFUN(b2_body_set_bullet);
CK_DLL_SFUN(b2_body_is_bullet);
CK_DLL_SFUN(b2_body_enable_hit_events);
CK_DLL_SFUN(b2_body_get_shape_count);
CK_DLL_SFUN(b2_body_get_shapes);
CK_DLL_SFUN(b2_body_get_joint_count);
CK_DLL_SFUN(b2_body_get_contact_capacity);
CK_DLL_SFUN(b2_body_compute_aabb);

void ulib_box2d_query(Chuck_DL_Query* QUERY)
{
    // b2_BodyType --------------------------------------
    BEGIN_CLASS("b2_BodyType", "Object");
    static t_CKINT b2_static_body    = 0;
    static t_CKINT b2_kinematic_body = 1;
    static t_CKINT b2_dynamic_body   = 2;
    SVAR("int", "staticBody", &b2_static_body);
    DOC_VAR("zero mass, zero velocity, may be manually moved");
    SVAR("int", "kinematicBody", &b2_kinematic_body);
    DOC_VAR("zero mass, velocity set by user, moved by solver");
    SVAR("int", "dynamicBody", &b2_dynamic_body);
    DOC_VAR("positive mass, velocity determined by forces, moved by solver");
    END_CLASS();

    // b2_ShapeType --------------------------------------
    BEGIN_CLASS("b2_ShapeType", "Object");
    static t_CKINT b2_circle_shape         = b2ShapeType::b2_circleShape;
    static t_CKINT b2_capsule_shape        = b2ShapeType::b2_capsuleShape;
    static t_CKINT b2_segment_shape        = b2ShapeType::b2_segmentShape;
    static t_CKINT b2_polygon_shape        = b2ShapeType::b2_polygonShape;
    static t_CKINT b2_smooth_segment_shape = b2ShapeType::b2_smoothSegmentShape;

    SVAR("int", "circleShape", &b2_circle_shape);
    DOC_VAR("A circle with an offset");

    SVAR("int", "capsuleShape", &b2_capsule_shape);
    DOC_VAR("A capsule is an extruded circle");

    SVAR("int", "segmentShape", &b2_segment_shape);
    DOC_VAR("A line segment");

    SVAR("int", "polygonShape", &b2_polygon_shape);
    DOC_VAR("A convex polygon");

    SVAR("int", "smoothSegmentShape", &b2_smooth_segment_shape);
    DOC_VAR("A smooth segment owned by a chain shape");

    END_CLASS();

    // b2_BodyDef --------------------------------------
    BEGIN_CLASS("b2_BodyDef", "Object");
    DOC_CLASS("https://box2d.org/documentation_v3/group__body.html#structb2_body_def");

    CTOR(b2_BodyDef_ctor);

    b2_BodyDef_type_offset = MVAR("int", "type", false);
    DOC_VAR("The body type: static, kinematic, or dynamic. Pass a b2_BodyType enum");

    b2_BodyDef_position_offset = MVAR("vec2", "position", false);
    DOC_VAR(
      "The initial world position of the body. Bodies should be created with "
      "the desired position. note Creating bodies at the origin and then "
      "moving them nearly doubles the cost of body creation, especially if the "
      "body is moved after shapes have been added.");

    b2_BodyDef_angle_offset = MVAR("float", "angle", false);
    DOC_VAR("The initial world angle of the body in radians.");

    b2_BodyDef_linearVelocity_offset = MVAR("vec2", "linearVelocity", false);
    DOC_VAR(
      "The initial linear velocity of the body's origin. Typically in meters "
      "per second.");

    b2_BodyDef_angularVelocity_offset = MVAR("float", "angularVelocity", false);
    DOC_VAR(
      "The initial angular velocity of the body. Typically in meters per "
      "second.");

    b2_BodyDef_linearDamping_offset = MVAR("float", "linearDamping", false);
    DOC_VAR(
      "Linear damping is use to reduce the linear velocity. The damping "
      "parameter can be larger than 1 but the damping effect becomes sensitive "
      "to the time step when the damping parameter is large. Generally linear "
      "damping is undesirable because it makes objects move slowly as if they "
      "are floating.");

    b2_BodyDef_angularDamping_offset = MVAR("float", "angularDamping", false);
    DOC_VAR(
      "Angular damping is use to reduce the angular velocity. The damping "
      "parameter can be larger than 1.0f but the damping effect becomes "
      "sensitive to the time step when the damping parameter is large. Angular "
      "damping can be use slow down rotating bodies.");

    b2_BodyDef_gravityScale_offset = MVAR("float", "gravityScale", false);
    DOC_VAR("Scale the gravity applied to this body. Non-dimensional.");

    b2_BodyDef_sleepThreshold_offset = MVAR("float", "sleepThreshold", false);
    DOC_VAR("Sleep velocity threshold, default is 0.05 meter per second");

    b2_BodyDef_enableSleep_offset = MVAR("int", "enableSleep", false);
    DOC_VAR("Set this flag to false if this body should never fall asleep.");

    b2_BodyDef_isAwake_offset = MVAR("int", "isAwake", false);
    DOC_VAR("Is this body initially awake or sleeping?");

    b2_BodyDef_fixedRotation_offset = MVAR("int", "fixedRotation", false);
    DOC_VAR("Should this body be prevented from rotating? Useful for characters.");

    b2_BodyDef_isBullet_offset = MVAR("int", "isBullet", false);
    DOC_VAR(
      "Treat this body as high speed object that performs continuous collision "
      "detection against dynamic and kinematic bodies, but not other bullet "
      "bodies. Warning Bullets should be used sparingly. They are not a "
      "solution for general dynamic-versus-dynamic continuous collision. They "
      "may interfere with joint constraints.");

    b2_BodyDef_isEnabled_offset = MVAR("int", "isEnabled", false);
    DOC_VAR("Used to disable a body. A disabled body does not move or collide.");

    b2_BodyDef_automaticMass_offset = MVAR("int", "automaticMass", false);
    DOC_VAR(
      "Automatically compute mass and related properties on this body from "
      "shapes. Triggers whenever a shape is add/removed/changed. Default is "
      "true.");

    END_CLASS(); // b2_BodyDef

    // b2_CastOutput --------------------------------------
    BEGIN_CLASS("b2_CastOutput", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/"
      "group__geometry.html#structb2_cast_output");

    b2_CastOutput_normal_offset = MVAR("vec2", "normal", false);
    DOC_VAR("The surface normal at the hit point");

    b2_CastOutput_point_offset = MVAR("vec2", "point", false);
    DOC_VAR("The surface hit point");

    b2_CastOutput_fraction_offset = MVAR("float", "fraction", false);
    DOC_VAR("The fraction of the input translation at collision");

    b2_CastOutput_iterations_offset = MVAR("int", "iterations", false);
    DOC_VAR("The number of iterations used");

    b2_CastOutput_hit_offset = MVAR("int", "hit", false);
    DOC_VAR("Did the cast hit?");

    END_CLASS(); // b2_CastOutput

    // b2_Polygon --------------------------------------
    // TODO eventually expose internals
    BEGIN_CLASS("b2_Polygon", "Object");
    DOC_CLASS(
      "Don't instantiate directly. Use helpers b2_Polygon.make* instead. "
      "https://box2d.org/documentation_v3/group__geometry.html#structb2_polygon"
      "A solid convex polygon. It is assumed that the interior of the polygon"
      "is to the left of each edge."
      "Polygons have a maximum number of vertices equal to "
      "b2_maxPolygonVertices."
      "In most cases you should not need many vertices for a convex polygon."
      "@warning DO NOT fill this out manually, instead use a helper function "
      "like b2MakePolygon or b2MakeBox.");

    b2_polygon_data_offset = MVAR("int", "@b2_polygon_data", false);

    DTOR(b2_polygon_dtor);

    // helpers
    SFUN(b2_polygon_make_box, "b2_Polygon", "makeBox");
    ARG("float", "hx");
    ARG("float", "hy");
    DOC_FUNC("Make a box (rectangle) polygon, bypassing the need for a convex hull.");

    // TODO other helpers

    END_CLASS(); // b2_Polygon

    // b2_Circle --------------------------------------
    BEGIN_CLASS("b2_Circle", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/"
      "group__geometry.html#structb2_circle");

    CTOR(b2_Circle_ctor);
    ARG("vec2", "center");
    ARG("float", "radius");

    b2_Circle_position_offset = MVAR("vec2", "center", false);
    DOC_VAR("The local center (relative to the body's origin)");

    b2_Circle_radius_offset = MVAR("float", "radius", false);
    DOC_VAR("The radius");

    END_CLASS(); // b2_Circle

    // b2_Capsule --------------------------------------
    BEGIN_CLASS("b2_Capsule", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/"
      "group__geometry.html#structb2_capsule");

    CTOR(b2_Capsule_ctor);
    ARG("vec2", "center1");
    ARG("vec2", "center2");
    ARG("float", "radius");

    b2_Capsule_center1_offset = MVAR("vec2", "center1", false);
    DOC_VAR("Local center of the first semicircle");

    b2_Capsule_center2_offset = MVAR("vec2", "center2", false);
    DOC_VAR("Local center of the second semicircle");

    b2_Capsule_radius_offset = MVAR("float", "radius", false);
    DOC_VAR("The radius of the semicircles");

    END_CLASS(); // b2_Capsule

    // b2_Segment --------------------------------------
    BEGIN_CLASS("b2_Segment", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/"
      "group__geometry.html#structb2_segment");

    CTOR(b2_Segment_ctor);
    ARG("vec2", "point1");
    ARG("vec2", "point2");

    b2_Segment_point1_offset = MVAR("vec2", "point1", false);
    DOC_VAR("The first point");

    b2_Segment_point2_offset = MVAR("vec2", "point2", false);
    DOC_VAR("The second point");

    END_CLASS(); // b2_Segment

    // b2_Filter --------------------------------------
    BEGIN_CLASS("b2_Filter", "Object");
    DOC_CLASS("https://box2d.org/documentation_v3/group__shape.html#structb2_filter");

    CTOR(b2_Filter_ctor);

    b2_Filter_categoryBits_offset = MVAR("int", "categoryBits", false);
    DOC_VAR("");

    b2_Filter_maskBits_offset = MVAR("int", "maskBits", false);
    DOC_VAR("");

    b2_Filter_groupIndex_offset = MVAR("int", "groupIndex", false);
    DOC_VAR("");

    END_CLASS(); // b2_Filter

    // b2_ShapeDef --------------------------------------
    BEGIN_CLASS("b2_ShapeDef", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/"
      "group__shape.html#structb2_shape_def");

    CTOR(b2_ShapeDef_ctor);

    b2_ShapeDef_friction_offset = MVAR("float", "friction", false);
    DOC_VAR("The Coulomb (dry) friction coefficient, usually in the range [0,1]");

    b2_ShapeDef_restitution_offset = MVAR("float", "restitution", false);
    DOC_VAR("The restitution (bounce) usually in the range [0,1]");

    b2_ShapeDef_density_offset = MVAR("float", "density", false);
    DOC_VAR("The density, usually in kg/m^2");

    b2_ShapeDef_filter_offset = MVAR("b2_Filter", "filter", false);
    DOC_VAR("Collision filtering data");

    b2_ShapeDef_isSensor_offset = MVAR("int", "isSensor", false);
    DOC_VAR(
      "A sensor shape generates overlap events but never generates a collision "
      "response");

    b2_ShapeDef_enableSensorEvents_offset = MVAR("int", "enableSensorEvents", false);
    DOC_VAR(
      "Enable sensor events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors");

    b2_ShapeDef_enableContactEvents_offset = MVAR("int", "enableContactEvents", false);
    DOC_VAR(
      "Enable contact events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors");

    b2_ShapeDef_enableHitEvents_offset = MVAR("int", "enableHitEvents", false);
    DOC_VAR(
      "Enable hit events for this shape. Only applies to kinematic and dynamic "
      "bodies. Ignored for sensors");

    b2_ShapeDef_enablePreSolveEvents_offset
      = MVAR("int", "enablePreSolveEvents", false);
    DOC_VAR(
      "Enable pre-solve contact events for this shape. Only applies to dynamic "
      "bodies. These are expensive and must be carefully handled due to "
      "threading. Ignored for sensors");

    b2_ShapeDef_forceContactCreation_offset
      = MVAR("int", "forceContactCreation", false);
    DOC_VAR(
      "Normally shapes on static bodies don't invoke contact creation when "
      "they are added to the world. This overrides that behavior and causes "
      "contact creation. This significantly slows down static body creation "
      "which can be important when there are many static shapes");

    END_CLASS(); // b2_ShapeDef

    // b2_WorldDef --------------------------------------
    BEGIN_CLASS("b2_WorldDef", "Object");
    DOC_CLASS("World definition used to create a simulation world.");

    CTOR(b2_WorldDef_ctor);

    b2_WorldDef_gravity_offset = MVAR("vec2", "gravity", false);
    DOC_VAR("Gravity vector. Box2D has no up-vector defined.");

    b2_WorldDef_restitutionThreshold_offset
      = MVAR("float", "restitutionThreshold", false);
    DOC_VAR(
      "Restitution velocity threshold, usually in m/s. Collisions above this "
      "speed have restitution applied (will bounce).");

    b2_WorldDef_contactPushoutVelocity_offset
      = MVAR("float", "contactPushoutVelocity", false);
    DOC_VAR(
      "This parameter controls how fast overlap is resolved and has units of "
      "meters per second");

    b2_WorldDef_hitEventThreshold_offset = MVAR("float", "hitEventThreshold", false);
    DOC_VAR("Threshold velocity for hit events. Usually meters per second.");

    b2_WorldDef_contactHertz_offset = MVAR("float", "contactHertz", false);
    DOC_VAR("Contact stiffness. Cycles per second.");

    b2_WorldDef_contactDampingRatio_offset
      = MVAR("float", "contactDampingRatio", false);
    DOC_VAR("Contact bounciness. Non-dimensional.");

    b2_WorldDef_jointHertz_offset = MVAR("float", "jointHertz", false);
    DOC_VAR("Joint stiffness. Cycles per second.");

    b2_WorldDef_jointDampingRatio_offset = MVAR("float", "jointDampingRatio", false);
    DOC_VAR("Joint bounciness. Non-dimensional.");

    b2_WorldDef_enableSleep_offset = MVAR("int", "enableSleep", false);
    DOC_VAR("Can bodies go to sleep to improve performance");

    b2_WorldDef_enableContinous_offset = MVAR("int", "enableContinuous", false);
    DOC_VAR("Enable continuous collision");

    b2_WorldDef_workerCount_offset = MVAR("int", "workerCount", false);
    DOC_VAR(
      "Number of workers to use with the provided task system. Box2D performs "
      "best when using only performance cores and accessing a single L2 cache. "
      "Efficiency cores and hyper-threading provide little benefit and may "
      "even harm performance.");

    END_CLASS(); // b2_WorldDef

    // b2BodyMoveEvent --------------------------------------
    Arena::init(&b2_body_move_event_pool, sizeof(Chuck_Object*) * 1024);

    BEGIN_CLASS("b2_BodyMoveEvent", "Object");
    // clang-format off
    DOC_CLASS(
      "https://box2d.org/documentation_v3/group__events.html#structb2_body_move_event"
      "Body move events triggered when a body moves. Triggered when a body "
      "moves due to simulation. Not reported for bodies moved by the user. "
      "This also has a flag to indicate that the body went to sleep so the "
      "application can also sleep that actor/entity/object associated with the "
      "body. On the other hand if the flag does not indicate the body went to "
      "sleep then the application can treat the actor/entity/object associated "
      "with the body as awake. This is an efficient way for an application to "
      "update game object transforms rather than calling functions such as "
      "b2Body_GetTransform() because this data is delivered as a contiguous "
      "array and it is only populated with bodies that have moved. @note If "
      "sleeping is disabled all dynamic and kinematic bodies will trigger move "
      "events.");
    // clang-format on

    b2_BodyMoveEvent_pos_offset = MVAR("vec2", "pos", false);
    DOC_VAR("2d position");

    b2_BodyMoveEvent_rot_offset = MVAR("float", "rot", false);
    DOC_VAR("rotation in radians around the z-axis");

    b2_BodyMoveEvent_bodyId_offset = MVAR("int", "bodyId", false);
    DOC_VAR("id of the b2body");

    b2_BodyMoveEvent_fellAsleep_offset = MVAR("int", "fellAsleep", false);

    END_CLASS(); // b2_BodyMoveEvent

    // b2_ContactHitEvent --------------------------------------
    Arena::init(&b2_contact_hit_event_pool, sizeof(Chuck_Object*) * 1024);

    BEGIN_CLASS("b2_ContactHitEvent", "Object");
    // clang-format off
    DOC_CLASS(
      "https://box2d.org/documentation_v3/group__events.html#structb2_contact_hit_event"
      "Don't instantiate directly. Use b2_World.contactEvents() instead"
    );
    // clang-format on

    b2_ContactHitEvent_shapeIdA_offset = MVAR("int", "shapeIdA", false);
    DOC_VAR("Id of the first shape");

    b2_ContactHitEvent_shapeIdB_offset = MVAR("int", "shapeIdB", false);
    DOC_VAR("Id of the second shape");

    b2_ContactHitEvent_point_offset = MVAR("vec2", "point", false);
    DOC_VAR("Point where the shapes hit");

    b2_ContactHitEvent_normal_offset = MVAR("vec2", "normal", false);
    DOC_VAR("Normal vector pointing from shape A to shape B");

    b2_ContactHitEvent_approachSpeed_offset = MVAR("float", "approachSpeed", false);
    DOC_VAR(
      "The speed the shapes are approaching. Always positive. Typically in "
      "meters per second.");

    END_CLASS(); // b2_ContactHitEvent

    // b2 --------------------------------------
    BEGIN_CLASS("b2", "Object");
    DOC_CLASS("documentation: https://box2d.org/");

    SFUN(chugl_set_b2_world, "void", "world");
    ARG("int", "world");
    DOC_FUNC("Set the active physics world for simulation");

    SFUN(b2_set_substep_count, "void", "substeps");
    ARG("int", "substeps");
    DOC_FUNC(
      "Set the number of substeps for the physics simulation. Increasing the "
      "substep count can increase accuracy. Default 4.");

    SFUN(b2_CreateWorld, "int", "createWorld");
    ARG("b2_WorldDef", "def");
    DOC_FUNC(
      "Create a world for rigid body simulation. A world contains bodies, "
      "shapes, and constraints. You make create up to 128 worlds. Each world "
      "is completely independent and may be simulated in parallel.");

    SFUN(b2_DestroyWorld, "void", "destroyWorld");
    ARG("int", "world_id");
    DOC_FUNC(
      "Destroy a world and all its contents. This will free all memory "
      "associated with the world, including bodies, shapes, and joints.");

    SFUN(b2_CreateBody, "int", "createBody");
    ARG("int", "world_id");
    ARG("b2_BodyDef", "def");
    DOC_FUNC(
      "Create a rigid body given a definition. No reference to the definition "
      "is retained. So you can create the definition on the stack and pass it "
      "as a pointer.");

    SFUN(b2_DestroyBody, "void", "destroyBody");
    ARG("int", "body_id");
    DOC_FUNC(
      "Destroy a rigid body given an id. This destroys all shapes and joints "
      "attached to the body. Do not keep references to the associated shapes "
      "and joints.");

    END_CLASS(); // b2

    // b2_Body ---------------------------------------
    BEGIN_CLASS("b2_Body", "Object");
    DOC_CLASS(
      " Don't create bodies directly. Use b2_World.createBody instead. "
      "https://box2d.org/documentation_v3/group__body.html");

    SFUN(b2_body_is_valid, "int", "isValid");
    ARG("int", "b2_body_id");
    DOC_FUNC("Check if a body id is valid");

    SFUN(b2_body_get_type, "int", "type");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the body type: static, kinematic, or dynamic");

    SFUN(b2_body_set_type, "void", "type");
    ARG("int", "b2_body_id");
    ARG("int", "b2_BodyType");
    DOC_FUNC(
      " Change the body type. This is an expensive operation. This "
      "automatically updates the mass properties regardless of the automatic "
      "mass setting.");

    SFUN(b2_body_get_position, "vec2", "position");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the world position of a body. This is the location of the body "
      "origin.");

    SFUN(b2_body_get_rotation, "complex", "rotation");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the world rotation of a body as a cosine/sine pair (complex "
      "number)");

    SFUN(b2_body_get_angle, "float", "angle");
    ARG("int", "b2_body_id");
    DOC_FUNC(" Get the body angle in radians in the range [-pi, pi]");

    SFUN(b2_body_set_transform, "void", "transform");
    ARG("int", "b2_body_id");
    ARG("vec2", "position");
    ARG("float", "angle");
    DOC_FUNC(
      "Set the world transform of a body. This acts as a teleport and is "
      "fairly expensive."
      "@note Generally you should create a body with then intended "
      "transform."
      "@see b2_BodyDef.position and b2_BodyDef.angle");

    SFUN(b2_body_get_local_point, "vec2", "localPoint");
    ARG("int", "b2_body_id");
    ARG("vec2", "worldPoint");
    DOC_FUNC("Get a local point on a body given a world point");

    SFUN(b2_body_get_world_point, "vec2", "worldPoint");
    ARG("int", "b2_body_id");
    ARG("vec2", "localPoint");
    DOC_FUNC("Get a world point on a body given a local point");

    SFUN(b2_body_get_local_vector, "vec2", "localVector");
    ARG("int", "b2_body_id");
    ARG("vec2", "worldVector");
    DOC_FUNC("Get a local vector on a body given a world vector");

    SFUN(b2_body_get_world_vector, "vec2", "worldVector");
    ARG("int", "b2_body_id");
    ARG("vec2", "localVector");
    DOC_FUNC("Get a world vector on a body given a local vector");

    SFUN(b2_body_get_linear_velocity, "vec2", "linearVelocity");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the linear velocity of a body's center of mass. Typically in "
      "meters per second.");

    SFUN(b2_body_set_linear_velocity, "void", "linearVelocity");
    ARG("int", "b2_body_id");
    ARG("vec2", "linearVelocity");
    DOC_FUNC("Set the linear velocity of a body. Typically in meters per second.");

    SFUN(b2_body_get_angular_velocity, "float", "angularVelocity");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the angular velocity of a body in radians per second");

    SFUN(b2_body_set_angular_velocity, "void", "angularVelocity");
    ARG("int", "b2_body_id");
    ARG("float", "angularVelocity");
    DOC_FUNC("Set the angular velocity of a body in radians per second");

    SFUN(b2_body_apply_force, "void", "force");
    ARG("int", "b2_body_id");
    ARG("vec2", "force");
    ARG("vec2", "point");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a force at a world point. If the force is not applied at the "
      "center of mass, it will generate a torque and affect the angular "
      "velocity. This optionally wakes up the body."
      "The force is ignored if the body is not awake.");

    SFUN(b2_body_apply_force_to_center, "void", "force");
    ARG("int", "b2_body_id");
    ARG("vec2", "force");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a force to the center of mass. This optionally wakes up the "
      "body. The force is ignored if the body is not awake.");

    SFUN(b2_body_apply_torque, "void", "torque");
    ARG("int", "b2_body_id");
    ARG("float", "torque");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a torque. This affects the angular velocity without affecting "
      "the linear velocity."
      "This optionally wakes the body. The torque is ignored if the body "
      "is not awake.");

    SFUN(b2_body_apply_linear_impulse, "void", "impulse");
    ARG("int", "b2_body_id");
    ARG("vec2", "impulse");
    ARG("vec2", "point");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply an impulse at a point. This immediately modifies the velocity."
      "It also modifies the angular velocity if the point of application "
      "is not at the center of mass. This optionally wakes the body."
      "The impulse is ignored if the body is not awake."
      "@warning This should be used for one-shot impulses. If you need a "
      "steady force,"
      "use a force instead, which will work better with the sub-stepping "
      "solver.");

    SFUN(b2_body_apply_linear_impulse_to_center, "void", "impulse");
    ARG("int", "b2_body_id");
    ARG("vec2", "impulse");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply an impulse to the center of mass. This immediately modifies "
      "the velocity."
      "The impulse is ignored if the body is not awake. This optionally "
      "wakes the body."
      "@warning This should be used for one-shot impulses. If you need a "
      "steady force,"
      "use a force instead, which will work better with the sub-stepping "
      "solver.");

    SFUN(b2_body_apply_angular_impulse, "void", "impulse");
    ARG("int", "b2_body_id");
    ARG("float", "impulse");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply an angular impulse. The impulse is ignored if the body is not "
      "awake. This optionally wakes the body."
      "@warning This should be used for one-shot impulses. If you need a "
      "steady"
      "force, use a force instead, which will work better with the "
      "sub-stepping solver.");

    SFUN(b2_body_get_mass, "float", "mass");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the mass of the body, typically in kilograms");

    SFUN(b2_body_get_inertia, "float", "inertia");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the inertia tensor of the body, typically in kg*m^2");

    SFUN(b2_body_get_local_center_of_mass, "vec2", "localCenterOfMass");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the center of mass position of the body in local space");

    SFUN(b2_body_get_world_center_of_mass, "vec2", "worldCenterOfMass");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the center of mass position of the body in world space");

    // SFUN(b2_body_get_mass_data, "b2_MassData", "massData");
    // ARG("int", "b2_body_id");
    // DOC_FUNC("Get the mass data for a body");

    // SFUN(b2_body_set_mass_data, "void", "massData");
    // ARG("int", "b2_body_id");
    // ARG("b2_MassData", "massData");
    // DOC_FUNC(
    //   "Override the body's mass properties. Normally this is computed "
    //   "automatically using the shape geometry and density."
    //   "This information is lost if a shape is added or removed or if the "
    //   "body type changes.");

    SFUN(b2_body_apply_mass_from_shapes, "void", "applyMassFromShapes");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "This update the mass properties to the sum of the mass properties "
      "of the shapes. This normally does not need to be called unless you "
      "called .massData(b2_MassData) to override the mass and you later want "
      "to reset "
      "the mass."
      "You may also use this when automatic mass computation has been "
      "disabled."
      "You should call this regardless of body type.");

    // automatic mass not yet added to box2c impl
    // SFUN(b2_body_set_automatic_mass, "void", "automaticMass");
    // ARG("int", "automaticMass");
    // DOC_FUNC(
    //   "Set the automatic mass setting. Normally this is set in b2BodyDef "
    //   "before creation.");

    // SFUN(b2_body_get_automatic_mass, "int", "automaticMass");
    // DOC_FUNC("Get the automatic mass setting");

    SFUN(b2_body_set_linear_damping, "void", "linearDamping");
    ARG("int", "b2_body_id");
    ARG("float", "linearDamping");
    DOC_FUNC(
      "Adjust the linear damping. Normally this is set in b2BodyDef before "
      "creation.");

    SFUN(b2_body_get_linear_damping, "float", "linearDamping");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the current linear damping.");

    SFUN(b2_body_set_angular_damping, "void", "angularDamping");
    ARG("int", "b2_body_id");
    ARG("float", "angularDamping");
    DOC_FUNC(
      "Adjust the angular damping. Normally this is set in b2BodyDef before "
      "creation.");

    SFUN(b2_body_get_angular_damping, "float", "angularDamping");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the current angular damping.");

    SFUN(b2_body_set_gravity_scale, "void", "gravityScale");
    ARG("int", "b2_body_id");
    ARG("float", "gravityScale");
    DOC_FUNC(
      "Adjust the gravity scale. Normally this is set in b2BodyDef before "
      "creation."
      "@see b2BodyDef::gravityScale");

    SFUN(b2_body_get_gravity_scale, "float", "gravityScale");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the current gravity scale");

    SFUN(b2_body_is_awake, "int", "isAwake");
    ARG("int", "b2_body_id");
    DOC_FUNC("Returns true if this body is awake");

    SFUN(b2_body_set_awake, "void", "awake");
    ARG("int", "b2_body_id");
    ARG("int", "awake");
    DOC_FUNC(
      "Wake a body from sleep. This wakes the entire island the body is "
      "touching."
      "@warning Putting a body to sleep will put the entire island of "
      "bodies"
      "touching this body to sleep, which can be expensive and possibly "
      "unintuitive.");

    SFUN(b2_body_enable_sleep, "void", "enableSleep");
    ARG("int", "b2_body_id");
    ARG("int", "enableSleep");
    DOC_FUNC(
      "Enable or disable sleeping for this body. If sleeping is disabled "
      "the body will wake.");

    SFUN(b2_body_is_sleep_enabled, "int", "isSleepEnabled");
    ARG("int", "b2_body_id");
    DOC_FUNC("Returns true if sleeping is enabled for this body");

    SFUN(b2_body_set_sleep_threshold, "void", "sleepThreshold");
    ARG("int", "b2_body_id");
    ARG("float", "sleepThreshold");
    DOC_FUNC("Set the sleep threshold, typically in meters per second");

    SFUN(b2_body_get_sleep_threshold, "float", "sleepThreshold");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the sleep threshold, typically in meters per second.");

    SFUN(b2_body_is_enabled, "int", "isEnabled");
    ARG("int", "b2_body_id");
    DOC_FUNC("Returns true if this body is enabled");

    SFUN(b2_body_disable, "void", "disable");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Disable a body by removing it completely from the simulation. This "
      "is expensive.");

    SFUN(b2_body_enable, "void", "enable");
    ARG("int", "b2_body_id");
    DOC_FUNC("Enable a body by adding it to the simulation. This is expensive.");

    SFUN(b2_body_set_fixed_rotation, "void", "fixedRotation");
    ARG("int", "b2_body_id");
    ARG("int", "flag");
    DOC_FUNC(
      "Set this body to have fixed rotation. This causes the mass to be "
      "reset in all cases.");

    SFUN(b2_body_is_fixed_rotation, "int", "isFixedRotation");
    ARG("int", "b2_body_id");
    DOC_FUNC("Does this body have fixed rotation?");

    SFUN(b2_body_set_bullet, "void", "bullet");
    ARG("int", "b2_body_id");
    ARG("int", "flag");
    DOC_FUNC(
      "Set this body to be a bullet. A bullet does continuous collision "
      "detection against dynamic bodies (but not other bullets).");

    SFUN(b2_body_is_bullet, "int", "isBullet");
    ARG("int", "b2_body_id");
    DOC_FUNC("Is this body a bullet?");

    SFUN(b2_body_enable_hit_events, "void", "enableHitEvents");
    ARG("int", "b2_body_id");
    ARG("int", "enableHitEvents");
    DOC_FUNC(
      "Enable/disable hit events on all shapes"
      "@see b2ShapeDef::enableHitEvents");

    SFUN(b2_body_get_shape_count, "int", "shapeCount");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the number of shapes on this body");

    SFUN(b2_body_get_shapes, "int[]", "shapes");
    ARG("int", "b2_body_id");
    ARG("int[]", "shape_id_array");
    DOC_FUNC("Get the shape ids for all shapes on this body");

    SFUN(b2_body_get_joint_count, "int", "jointCount");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the number of joints on this body");

    // TODO add after impl joints
    // SFUN(b2_body_get_joints, "b2_Joint[]", "joints");
    // DOC_FUNC("Get the joint ids for all joints on this body");

    SFUN(b2_body_get_contact_capacity, "int", "contactCapacity");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the maximum capacity required for retrieving all the touching "
      "contacts on a body");

    // TODO add after impl contacts
    // SFUN(b2_body_get_contact_data, "b2_ContactData[]", "contactData");
    // ARG("int", "b2_body_id");
    // DOC_FUNC("Get the touching contact data for a body");

    SFUN(b2_body_compute_aabb, "vec4", "computeAABB");
    // ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the current world AABB that contains all the attached shapes. Note "
      "that this may not encompass the body origin."
      "If there are no shapes attached then the returned AABB is empty and "
      "centered on the body origin."
      "The aabb is in the form of (lowerBound.x, lowerBound.y, upperBound.x, "
      "upperBound.y");

    END_CLASS(); // b2_Body

    BEGIN_CLASS("b2_Shape", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/group__shape.html"
      "Don't instantiate directly. Use b2_Shape.createXXXShape() functions "
      "instead");

    SFUN(b2_CreateCircleShape, "int", "createCircleShape");
    ARG("int", "body_id");
    ARG("b2_ShapeDef", "def");
    ARG("b2_Circle", "circle");
    DOC_FUNC(
      "Create a circle shape and attach it to a body. The shape definition and "
      "geometry are fully cloned. Contacts are not created until the next time "
      "step.@return the shape id for accessing the shape");

    SFUN(b2_CreateSegmentShape, "int", "createSegmentShape");
    ARG("int", "body_id");
    ARG("b2_ShapeDef", "def");
    ARG("b2_Segment", "segment");
    DOC_FUNC(
      "Create a line segment shape and attach it to a body. The shape "
      "definition and geometry are fully cloned. Contacts are not created "
      "until the next time step."
      "@return the shape id for accessing the shape");

    SFUN(b2_CreateCapsuleShape, "int", "createCapsuleShape");
    ARG("int", "body_id");
    ARG("b2_ShapeDef", "def");
    ARG("b2_Capsule", "capsule");
    DOC_FUNC(
      "Create a capsule shape and attach it to a body. The shape definition "
      "and geometry are fully cloned. Contacts are not created until the next "
      "time step. @return the shape id for accessing the shape");

    SFUN(b2_CreatePolygonShape, "int", "createPolygonShape");
    ARG("int", "body_id");
    ARG("b2_ShapeDef", "def");
    ARG("b2_Polygon", "polygon");
    DOC_FUNC(
      "Create a polygon shape and attach it to a body. The shape definition "
      "and geometry are fully cloned. Contacts are not created until the next "
      "time step.  @return the shape id for accessing the shape");

    // shape accessors

    SFUN(b2_Shape_IsValid, "int", "isValid");
    ARG("int", "shape_id");
    DOC_FUNC("Check if a shape id is valid");

    SFUN(b2_Shape_GetType, "int", "type");
    ARG("int", "shape_id");
    DOC_FUNC("Get the type of a shape. Returns b2_ShapeType");

    SFUN(b2_Shape_GetBody, "int", "body");
    ARG("int", "shape_id");
    DOC_FUNC("Get the body id that a shape is attached to");

    SFUN(b2_Shape_IsSensor, "int", "isSensor");
    ARG("int", "shape_id");
    DOC_FUNC("Returns true If the shape is a sensor");

    SFUN(b2_Shape_SetDensity, "void", "density");
    ARG("int", "shape_id");
    ARG("float", "density");
    DOC_FUNC(
      "Set the mass density of a shape, typically in kg/m^2. This will not "
      "update the mass properties on the parent body.");

    SFUN(b2_Shape_GetDensity, "float", "density");
    ARG("int", "shape_id");
    DOC_FUNC("Get the density of a shape, typically in kg/m^2");

    SFUN(b2_Shape_SetFriction, "void", "friction");
    ARG("int", "shape_id");
    ARG("float", "friction");
    DOC_FUNC("Set the friction on a shape");

    SFUN(b2_Shape_GetFriction, "float", "friction");
    ARG("int", "shape_id");
    DOC_FUNC("Get the friction of a shape");

    SFUN(b2_Shape_SetRestitution, "void", "restitution");
    ARG("int", "shape_id");
    ARG("float", "restitution");
    DOC_FUNC("Set the shape restitution (bounciness)");

    SFUN(b2_Shape_GetRestitution, "float", "restitution");
    ARG("int", "shape_id");
    DOC_FUNC("Get the shape restitution");

    SFUN(b2_Shape_SetFilter, "void", "filter");
    ARG("int", "shape_id");
    ARG("b2_Filter", "filter");
    DOC_FUNC(
      "Set the current filter. This is almost as expensive as recreating the "
      "shape. @see b2_ShapeDef::filter");

    SFUN(b2_Shape_GetFilter, "b2_Filter", "filter");
    ARG("int", "shape_id");
    DOC_FUNC("Get the shape filter");

    SFUN(b2_Shape_EnableSensorEvents, "void", "enableSensorEvents");
    ARG("int", "shape_id");
    ARG("int", "flag");
    DOC_FUNC(
      "Enable sensor events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors. @see b2_ShapeDef::isSensor");

    SFUN(b2_Shape_AreSensorEventsEnabled, "int", "areSensorEventsEnabled");
    ARG("int", "shape_id");
    DOC_FUNC("Returns true if sensor events are enabled");

    SFUN(b2_Shape_EnableContactEvents, "void", "enableContactEvents");
    ARG("int", "shape_id");
    ARG("int", "flag");
    DOC_FUNC(
      "Enable contact events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors. @see "
      "b2ShapeDef::enableContactEvents");

    SFUN(b2_Shape_AreContactEventsEnabled, "int", "areContactEventsEnabled");
    ARG("int", "shape_id");
    DOC_FUNC("Returns true if contact events are enabled");

    SFUN(b2_Shape_EnablePreSolveEvents, "void", "enablePreSolveEvents");
    ARG("int", "shape_id");
    ARG("int", "flag");
    DOC_FUNC(
      "Enable pre-solve contact events for this shape. Only applies to dynamic "
      "bodies. These are expensive and must be carefully handled due to "
      "multithreading. Ignored for sensors. @see b2PreSolveFcn");

    SFUN(b2_Shape_ArePreSolveEventsEnabled, "int", "arePreSolveEventsEnabled");
    ARG("int", "shape_id");
    DOC_FUNC("Returns true if pre-solve events are enabled");

    SFUN(b2_Shape_EnableHitEvents, "void", "enableHitEvents");
    ARG("int", "shape_id");
    ARG("int", "flag");
    DOC_FUNC(
      "Enable contact hit events for this shape. Ignored for sensors. @see "
      "b2WorldDef::hitEventThreshold");

    SFUN(b2_Shape_AreHitEventsEnabled, "int", "areHitEventsEnabled");
    ARG("int", "shape_id");
    DOC_FUNC("Returns true if hit events are enabled");

    SFUN(b2_Shape_TestPoint, "int", "testPoint");
    ARG("int", "shape_id");
    ARG("vec2", "point");
    DOC_FUNC("Test a point for overlap with a shape");

    SFUN(b2_Shape_RayCast, "void", "rayCast");
    ARG("int", "shape_id");
    ARG("vec2", "origin");
    ARG("vec2", "translation");
    ARG("b2_CastOutput", "output");
    DOC_FUNC(
      "Ray cast a shape directly. Copies the output to the provided "
      "output object.");

    SFUN(b2_Shape_GetCircle, "b2_Circle", "circle");
    ARG("int", "shape_id");
    DOC_FUNC("Get a copy of the shape's circle. Asserts the type is correct.");

    SFUN(b2_Shape_GetSegment, "b2_Segment", "segment");
    ARG("int", "shape_id");
    DOC_FUNC("Get a copy of the shape's line segment. Asserts the type is correct.");

    // TODO
    // SFUN(b2_Shape_GetSmoothSegment, "b2_SmoothSegment", "smoothSegment");
    // ARG("int", "shape_id");
    // DOC_FUNC(
    //   "Get a copy of the shape's smooth line segment. Asserts the type is "
    //   "correct.");

    SFUN(b2_Shape_GetCapsule, "b2_Capsule", "capsule");
    ARG("int", "shape_id");
    DOC_FUNC("Get a copy of the shape's capsule. Asserts the type is correct.");

    SFUN(b2_Shape_GetPolygon, "b2_Polygon", "polygon");
    ARG("int", "shape_id");
    DOC_FUNC("Get a copy of the shape's convex polygon. Asserts the type is correct.");

    SFUN(b2_Shape_SetCircle, "void", "circle");
    ARG("int", "shape_id");
    ARG("b2_Circle", "circle");
    DOC_FUNC(
      "Allows you to change a shape to be a circle or update the current "
      "circle."
      "This does not modify the mass properties."
      "@see b2Body_ApplyMassFromShapes");

    SFUN(b2_Shape_SetSegment, "void", "segment");
    ARG("int", "shape_id");
    ARG("b2_Segment", "segment");
    DOC_FUNC(
      "Allows you to change a shape to be a segment or update the current "
      "segment."
      "This does not modify the mass properties.");

    SFUN(b2_Shape_SetCapsule, "void", "capsule");
    ARG("int", "shape_id");
    ARG("b2_Capsule", "capsule");
    DOC_FUNC(
      "Allows you to change a shape to be a capsule or update the current "
      "capsule."
      "This does not modify the mass properties."
      "@see b2Body_ApplyMassFromShapes");

    SFUN(b2_Shape_SetPolygon, "void", "polygon");
    ARG("int", "shape_id");
    ARG("b2_Polygon", "polygon");
    DOC_FUNC(
      "Allows you to change a shape to be a polygon or update the current "
      "polygon."
      "This does not modify the mass properties."
      "@see b2Body_ApplyMassFromShapes");

    SFUN(b2_Shape_GetParentChain, "int", "parentChain");
    ARG("int", "shape_id");
    DOC_FUNC(
      "Get the parent chain id if the shape type is b2_smoothSegmentShape,"
      "otherwise returns b2_nullChainId = 0x00.");

    SFUN(b2_Shape_GetContactCapacity, "int", "contactCapacity");
    ARG("int", "shape_id");
    DOC_FUNC(
      "Get the maximum capacity required for retrieving all the touching "
      "contacts on a shape");

    // SFUN(b2_Shape_GetContactData, "void", "contactData");
    // ARG("int", "shape_id");
    // ARG("b2_ContactData[]", "contacts");
    // DOC_FUNC(
    //   "Get the touching contact data for a shape. The provided shapeId will
    //   be " "either shapeIdA or shapeIdB on the contact data. Contact data is
    //   copied " "into the given contacts array.");

    SFUN(b2_Shape_GetAABB, "vec4", "AABB");
    ARG("int", "shape_id");
    DOC_FUNC(
      "Get the current world AABB. Returns (lowerBound.x, lowerBound.y, "
      "upperBound.x, upperBound.y)");

    SFUN(b2_Shape_GetClosestPoint, "vec2", "closestPoint");
    ARG("int", "shape_id");
    ARG("vec2", "target");
    DOC_FUNC(
      "Get the closest point on a shape to a target point. Target and result "
      "are in world space.");

    END_CLASS(); // b2_Shape

    // b2_World --------------------------------------

    BEGIN_CLASS("b2_World", "Object");

    SFUN(b2_World_IsValid, "int", "isValid");
    ARG("int", "world_id");
    DOC_FUNC("World id validation. Provides validation for up to 64K allocations.");

    // TODO debug draw

    SFUN(b2_World_GetBodyEvents, "void", "bodyEvents");
    ARG("int", "world_id");
    ARG("b2_BodyMoveEvent[]", "body_events");
    DOC_FUNC("Get the body events for the current time step.");

    SFUN(b2_World_GetSensorEvents, "void", "sensorEvents");
    ARG("int", "world_id");
    ARG("int[]", "begin_sensor_events");
    ARG("int[]", "end_sensor_events");
    DOC_FUNC(
      "Original box2D documentation: "
      "https://box2d.org/documentation_v3/"
      "group__events.html#structb2_sensor_events "
      "This implementation is different and optimized for cache performance. "
      "The two input arrays,"
      "begin_sensor_events and end_sensor_events, are used to store the sensor "
      "events of the last frame,"
      "which are a pair of b2ShapeIds (int), stored in order. E.g. "
      "begin_sensor_events[i] is the id of the"
      "sensor shape, and begin_sensor_events[i+1] is the id of the dynamic "
      "shape that began contact with the sensor.");

    SFUN(b2_World_GetContactEvents, "void", "contactEvents");
    ARG("int", "world_id");
    ARG("int[]", "begin_contact_events_shape_ids");
    ARG("int[]", "end_contact_events_shape_ids");
    ARG("b2_ContactHitEvent[]", "contact_hit_events");
    DOC_FUNC(
      "https://box2d.org/documentation_v3/"
      "group__world.html#ga67e9e2ecf3897d4c7254196395be65ca"
      "Unlike the original box2D implementation, this function returns the "
      "ContactBeginTouchEvents and ContactEndTouchEvents in two separate flat "
      "arrays"
      "of b2ShapeIds (int), stored in order. E.g. begin_contact_events[i] and "
      "begin_contact_events[i+1] are the ids of the two shapes that began "
      "contact."
      "The contact_hit_events array is used to store the contact hit events of "
      "the "
      "last frame.");

    END_CLASS(); // b2_World
}

// ============================================================================
// b2BodyMoveEvent
// ============================================================================

static void b2BodyMoveEvent_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                     b2BodyMoveEvent* obj)
{
    OBJ_MEMBER_VEC2(ckobj, b2_BodyMoveEvent_pos_offset)
      = { obj->transform.p.x, obj->transform.p.y };
    // convert complex rot to radians
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyMoveEvent_rot_offset)
      = atan2(obj->transform.q.s, obj->transform.q.c);
    OBJ_MEMBER_B2_ID(b2BodyId, ckobj, b2_BodyMoveEvent_bodyId_offset) = obj->bodyId;
    OBJ_MEMBER_INT(ckobj, b2_BodyMoveEvent_fellAsleep_offset)         = obj->fellAsleep;
}

// ============================================================================
// b2
// ============================================================================

CK_DLL_SFUN(chugl_set_b2_world)
{
    b2WorldId world_id = GET_B2_ID(b2WorldId, ARGS);
    CQ_PushCommand_b2World_Set(*(u32*)&world_id);
}

CK_DLL_SFUN(b2_set_substep_count)
{
    CQ_PushCommand_b2SubstepCount(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_CreateWorld)
{
    b2WorldDef def = b2DefaultWorldDef();
    // TODO impl enqueueTask and finishTask callbacks
    ckobj_to_b2WorldDef(API, &def, GET_NEXT_OBJECT(ARGS));
    RETURN_B2_ID(b2WorldId, b2CreateWorld(&def));
}

CK_DLL_SFUN(b2_DestroyWorld)
{
    b2DestroyWorld(GET_B2_ID(b2WorldId, ARGS));
}

CK_DLL_SFUN(b2_CreateBody)
{
    b2WorldId world_id = GET_B2_ID(b2WorldId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg
    b2BodyDef body_def = b2DefaultBodyDef();
    ckobj_to_b2BodyDef(API, &body_def, GET_NEXT_OBJECT(ARGS));
    RETURN_B2_ID(b2BodyId, b2CreateBody(world_id, &body_def));
}

CK_DLL_SFUN(b2_DestroyBody)
{
    b2DestroyBody(GET_B2_ID(b2BodyId, ARGS));
}

// ============================================================================
// b2_World
// ============================================================================

CK_DLL_SFUN(b2_World_IsValid)
{
    RETURN->v_int = b2World_IsValid(GET_B2_ID(b2WorldId, ARGS));
}

CK_DLL_SFUN(b2_World_GetBodyEvents)
{
    b2WorldId world_id = GET_B2_ID(b2WorldId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg
    Chuck_ArrayInt* body_event_array = GET_NEXT_OBJECT_ARRAY(ARGS);

    b2BodyEvents body_events = b2World_GetBodyEvents(world_id);

    // first create new body events in pool
    int pool_len = ARENA_LENGTH(&b2_body_move_event_pool, Chuck_Object*);
    int diff     = body_events.moveCount - pool_len;
    if (diff > 0) {
        Chuck_Object** p
          = ARENA_PUSH_COUNT(&b2_body_move_event_pool, Chuck_Object*, diff);
        for (int i = 0; i < diff; i++)
            p[i] = chugin_createCkObj("b2_BodyMoveEvent", true, SHRED);
    }

    // TODO switch to use array_int_set after updating chuck version
    // int ck_array_len         = API->object->array_int_size(body_event_array);

    // clear array
    API->object->array_int_clear(body_event_array);

    // add new body events to array
    for (int i = 0; i < body_events.moveCount; i++) {
        Chuck_Object* ckobj
          = *ARENA_GET_TYPE(&b2_body_move_event_pool, Chuck_Object*, i);
        b2BodyMoveEvent_to_ckobj(API, ckobj, &body_events.moveEvents[i]);
        API->object->array_int_push_back(body_event_array, (t_CKUINT)ckobj);
    }
}

CK_DLL_SFUN(b2_World_GetSensorEvents)
{
    b2WorldId world_id = GET_B2_ID(b2WorldId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg
    Chuck_ArrayInt* begin_sensor_events = GET_NEXT_OBJECT_ARRAY(ARGS);
    Chuck_ArrayInt* end_sensor_events   = GET_NEXT_OBJECT_ARRAY(ARGS);

    // TODO switch to use array_int_set after updating chuck version

    b2SensorEvents sensor_events = b2World_GetSensorEvents(world_id);

    // clear arrays
    API->object->array_int_clear(begin_sensor_events);
    API->object->array_int_clear(end_sensor_events);

    // add new sensor events to arrays
    for (int i = 0; i < sensor_events.beginCount; i++) {
        API->object->array_int_push_back(
          begin_sensor_events,
          B2_ID_TO_CKINT(sensor_events.beginEvents[i].sensorShapeId));
        API->object->array_int_push_back(
          begin_sensor_events,
          B2_ID_TO_CKINT(sensor_events.beginEvents[i].visitorShapeId));
    }

    ASSERT(API->object->array_int_size(begin_sensor_events)
           == sensor_events.beginCount * 2);

    for (int i = 0; i < sensor_events.endCount; i++) {
        API->object->array_int_push_back(
          end_sensor_events, B2_ID_TO_CKINT(sensor_events.endEvents[i].sensorShapeId));
        API->object->array_int_push_back(
          end_sensor_events, B2_ID_TO_CKINT(sensor_events.endEvents[i].visitorShapeId));
    }

    ASSERT(API->object->array_int_size(end_sensor_events)
           == sensor_events.endCount * 2);
}

CK_DLL_SFUN(b2_World_GetContactEvents)
{
    b2WorldId world_id = GET_B2_ID(b2WorldId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg
    Chuck_ArrayInt* begin_contact_events = GET_NEXT_OBJECT_ARRAY(ARGS);
    Chuck_ArrayInt* end_contact_events   = GET_NEXT_OBJECT_ARRAY(ARGS);
    Chuck_ArrayInt* hit_events           = GET_NEXT_OBJECT_ARRAY(ARGS);

    // TODO switch to use array_int_set after updating chuck version
    API->object->array_int_clear(begin_contact_events);
    API->object->array_int_clear(end_contact_events);
    API->object->array_int_clear(hit_events);

    b2ContactEvents contact_events = b2World_GetContactEvents(world_id);

    // populate begin_contact_events and end_contact_events
    for (int i = 0; i < contact_events.beginCount; i++) {
        API->object->array_int_push_back(
          begin_contact_events, B2_ID_TO_CKINT(contact_events.beginEvents[i].shapeIdA));
        API->object->array_int_push_back(
          begin_contact_events, B2_ID_TO_CKINT(contact_events.beginEvents[i].shapeIdB));
    }
    for (int i = 0; i < contact_events.endCount; i++) {
        API->object->array_int_push_back(
          end_contact_events, B2_ID_TO_CKINT(contact_events.endEvents[i].shapeIdA));
        API->object->array_int_push_back(
          end_contact_events, B2_ID_TO_CKINT(contact_events.endEvents[i].shapeIdB));
    }

    // populate hit_events
    // first create new body events in pool
    int pool_len = ARENA_LENGTH(&b2_contact_hit_event_pool, Chuck_Object*);
    int diff     = contact_events.hitCount - pool_len;
    if (diff > 0) {
        Chuck_Object** p
          = ARENA_PUSH_COUNT(&b2_contact_hit_event_pool, Chuck_Object*, diff);
        for (int i = 0; i < diff; i++)
            p[i] = chugin_createCkObj("b2_ContactHitEvent", true, SHRED);
    }

    // add new body events to array
    for (int i = 0; i < contact_events.hitCount; i++) {
        Chuck_Object* ckobj
          = *ARENA_GET_TYPE(&b2_contact_hit_event_pool, Chuck_Object*, i);
        b2ContactHitEvent_to_ckobj(API, ckobj, &contact_events.hitEvents[i]);
        API->object->array_int_push_back(hit_events, (t_CKUINT)ckobj);
    }
}

// ============================================================================
// b2_WorldDef
// ============================================================================
static void b2WorldDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2WorldDef* obj)
{
    OBJ_MEMBER_VEC2(ckobj, b2_WorldDef_gravity_offset)
      = { obj->gravity.x, obj->gravity.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_restitutionThreshold_offset)
      = obj->restitutionThreshold;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactPushoutVelocity_offset)
      = obj->contactPushoutVelocity;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_hitEventThreshold_offset)
      = obj->hitEventThreshold;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactHertz_offset) = obj->contactHertz;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactDampingRatio_offset)
      = obj->contactDampingRatio;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_jointHertz_offset) = obj->jointHertz;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_jointDampingRatio_offset)
      = obj->jointDampingRatio;
    OBJ_MEMBER_INT(ckobj, b2_WorldDef_enableSleep_offset)     = obj->enableSleep;
    OBJ_MEMBER_INT(ckobj, b2_WorldDef_enableContinous_offset) = obj->enableContinous;
    OBJ_MEMBER_INT(ckobj, b2_WorldDef_workerCount_offset)     = obj->workerCount;
}

static void ckobj_to_b2WorldDef(CK_DL_API API, b2WorldDef* obj, Chuck_Object* ckobj)
{
    t_CKVEC2 gravity_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_WorldDef_gravity_offset);
    obj->gravity          = { (float)gravity_vec2.x, (float)gravity_vec2.y };
    obj->restitutionThreshold
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_restitutionThreshold_offset);
    obj->contactPushoutVelocity
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactPushoutVelocity_offset);
    obj->hitEventThreshold
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_hitEventThreshold_offset);
    obj->contactHertz = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactHertz_offset);
    obj->contactDampingRatio
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactDampingRatio_offset);
    obj->jointHertz = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_jointHertz_offset);
    obj->jointDampingRatio
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_jointDampingRatio_offset);
    obj->enableSleep     = OBJ_MEMBER_INT(ckobj, b2_WorldDef_enableSleep_offset);
    obj->enableContinous = OBJ_MEMBER_INT(ckobj, b2_WorldDef_enableContinous_offset);
    obj->workerCount     = OBJ_MEMBER_INT(ckobj, b2_WorldDef_workerCount_offset);
}

CK_DLL_CTOR(b2_WorldDef_ctor)
{
    b2WorldDef default_world_def = b2DefaultWorldDef();
    b2WorldDef_to_ckobj(API, SELF, &default_world_def);
}

// ============================================================================
// b2_Filter
// ============================================================================

static void b2Filter_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Filter* obj)
{
    OBJ_MEMBER_INT(ckobj, b2_Filter_categoryBits_offset) = obj->categoryBits;
    OBJ_MEMBER_INT(ckobj, b2_Filter_maskBits_offset)     = obj->maskBits;
    ASSERT(OBJ_MEMBER_INT(ckobj, b2_Filter_maskBits_offset) == obj->maskBits);
    OBJ_MEMBER_INT(ckobj, b2_Filter_groupIndex_offset) = obj->groupIndex;
}

static void ckobj_to_b2Filter(CK_DL_API API, b2Filter* obj, Chuck_Object* ckobj)
{
    obj->categoryBits = OBJ_MEMBER_INT(ckobj, b2_Filter_categoryBits_offset);
    obj->maskBits     = OBJ_MEMBER_INT(ckobj, b2_Filter_maskBits_offset);
    obj->groupIndex   = OBJ_MEMBER_INT(ckobj, b2_Filter_groupIndex_offset);
}

CK_DLL_CTOR(b2_Filter_ctor)
{
    b2Filter default_filter = b2DefaultFilter();
    b2Filter_to_ckobj(API, SELF, &default_filter);
}

// ============================================================================
// b2_ShapeDef
// ============================================================================

static void b2ShapeDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2ShapeDef* obj)
{
    OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_friction_offset)    = obj->friction;
    OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_restitution_offset) = obj->restitution;
    OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_density_offset)     = obj->density;
    // b2Filter_to_ckobj(API, OBJ_MEMBER_OBJECT(ckobj,
    // b2_ShapeDef_filter_offset),
    //                   &obj->filter);
    b2Filter_to_ckobj(API,
                      (Chuck_Object*)OBJ_MEMBER_UINT(ckobj, b2_ShapeDef_filter_offset),
                      &obj->filter);
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_isSensor_offset) = obj->isSensor;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableSensorEvents_offset)
      = obj->enableSensorEvents;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableContactEvents_offset)
      = obj->enableContactEvents;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableHitEvents_offset) = obj->enableHitEvents;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enablePreSolveEvents_offset)
      = obj->enablePreSolveEvents;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_forceContactCreation_offset)
      = obj->forceContactCreation;
}

static void ckobj_to_b2ShapeDef(CK_DL_API API, b2ShapeDef* obj, Chuck_Object* ckobj)
{
    obj->friction    = (float)OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_friction_offset);
    obj->restitution = (float)OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_restitution_offset);
    obj->density     = (float)OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_density_offset);
    ckobj_to_b2Filter(API, &obj->filter,
                      OBJ_MEMBER_OBJECT(ckobj, b2_ShapeDef_filter_offset));
    obj->isSensor = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_isSensor_offset);
    obj->enableSensorEvents
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableSensorEvents_offset);
    obj->enableContactEvents
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableContactEvents_offset);
    obj->enableHitEvents = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableHitEvents_offset);
    obj->enablePreSolveEvents
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enablePreSolveEvents_offset);
    obj->forceContactCreation
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_forceContactCreation_offset);
}

CK_DLL_CTOR(b2_ShapeDef_ctor)
{
    // https://github.com/ccrma/chuck/issues/449
    // member vars which are themselves Chuck_Objects are NOT instantiated
    // instantiating manually
    OBJ_MEMBER_OBJECT(SELF, b2_ShapeDef_filter_offset) = chugin_createCkObj(
      "b2_Filter", true, SHRED); // adding refcount just in case gc isn't set up

    b2ShapeDef default_shape_def = b2DefaultShapeDef();
    b2ShapeDef_to_ckobj(API, SELF, &default_shape_def);
}

// ============================================================================
// b2Polygon
// ============================================================================

CK_DLL_DTOR(b2_polygon_dtor)
{
    CHUGIN_SAFE_DELETE(b2Polygon, b2_polygon_data_offset);
}

CK_DLL_SFUN(b2_polygon_make_box)
{
    float hx          = GET_NEXT_FLOAT(ARGS);
    float hy          = GET_NEXT_FLOAT(ARGS);
    b2Polygon polygon = b2MakeBox(hx, hy);

    RETURN->v_object = b2_polygon_create(SHRED, &polygon);
}

// ============================================================================
// b2_Shape
// ============================================================================

CK_DLL_SFUN(b2_CreateCircleShape)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg

    b2ShapeDef shape_def = b2DefaultShapeDef();
    ckobj_to_b2ShapeDef(API, &shape_def, GET_NEXT_OBJECT(ARGS));

    Chuck_Object* circle_obj = GET_NEXT_OBJECT(ARGS);
    b2Circle circle          = {};
    ckobj_to_b2Circle(API, &circle, circle_obj);

    RETURN_B2_ID(b2ShapeId, b2CreateCircleShape(body_id, &shape_def, &circle));
}

CK_DLL_SFUN(b2_CreateSegmentShape)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg

    b2ShapeDef shape_def = b2DefaultShapeDef();
    ckobj_to_b2ShapeDef(API, &shape_def, GET_NEXT_OBJECT(ARGS));

    Chuck_Object* segment_obj = GET_NEXT_OBJECT(ARGS);
    b2Segment segment         = {};
    ckobj_to_b2Segment(API, &segment, segment_obj);

    RETURN_B2_ID(b2ShapeId, b2CreateSegmentShape(body_id, &shape_def, &segment));
}

CK_DLL_SFUN(b2_CreateCapsuleShape)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg

    b2ShapeDef shape_def = b2DefaultShapeDef();
    ckobj_to_b2ShapeDef(API, &shape_def, GET_NEXT_OBJECT(ARGS));

    Chuck_Object* capsule_obj = GET_NEXT_OBJECT(ARGS);
    b2Capsule capsule         = {};
    ckobj_to_b2Capsule(API, &capsule, capsule_obj);

    RETURN_B2_ID(b2ShapeId, b2CreateCapsuleShape(body_id, &shape_def, &capsule));
}

CK_DLL_SFUN(b2_CreatePolygonShape)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg

    b2ShapeDef shape_def = b2DefaultShapeDef();
    ckobj_to_b2ShapeDef(API, &shape_def, GET_NEXT_OBJECT(ARGS));

    b2Polygon* polygon
      = OBJ_MEMBER_B2_PTR(b2Polygon, GET_NEXT_OBJECT(ARGS), b2_polygon_data_offset);
    // b2Polygon polygon = b2MakeBox(.5f, .5f);

    RETURN_B2_ID(b2ShapeId, b2CreatePolygonShape(body_id, &shape_def, polygon));
    // RETURN_B2_ID(b2ShapeId,
    //              b2CreatePolygonShape(body_id, &shape_def, &polygon));
}

CK_DLL_SFUN(b2_Shape_IsValid)
{
    RETURN->v_int = b2Shape_IsValid(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_GetType)
{
    RETURN->v_int = b2Shape_GetType(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_GetBody)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    RETURN_B2_ID(b2BodyId, b2Shape_GetBody(shape_id));
}

CK_DLL_SFUN(b2_Shape_IsSensor)
{
    RETURN->v_int = b2Shape_IsSensor(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_SetDensity)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    float density = GET_NEXT_FLOAT(ARGS);
    b2Shape_SetDensity(shape_id, density);
}

CK_DLL_SFUN(b2_Shape_GetDensity)
{
    RETURN->v_float = b2Shape_GetDensity(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_SetFriction)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    float friction = GET_NEXT_FLOAT(ARGS);
    b2Shape_SetFriction(shape_id, friction);
}

CK_DLL_SFUN(b2_Shape_GetFriction)
{
    RETURN->v_float = b2Shape_GetFriction(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_SetRestitution)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    float f = GET_NEXT_FLOAT(ARGS);
    b2Shape_SetRestitution(shape_id, f);
}

CK_DLL_SFUN(b2_Shape_GetRestitution)
{
    RETURN->v_float = b2Shape_GetRestitution(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_GetFilter)
{
    b2Filter filter            = b2Shape_GetFilter(GET_B2_ID(b2ShapeId, ARGS));
    Chuck_Object* filter_ckobj = chugin_createCkObj("b2_Filter", false, SHRED);
    b2Filter_to_ckobj(API, filter_ckobj, &filter);
    RETURN->v_object = filter_ckobj;
}

CK_DLL_SFUN(b2_Shape_SetFilter)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Filter filter = b2DefaultFilter();
    ckobj_to_b2Filter(API, &filter, GET_NEXT_OBJECT(ARGS));
    b2Shape_SetFilter(shape_id, filter);
}

CK_DLL_SFUN(b2_Shape_EnableSensorEvents)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    int flag = GET_NEXT_INT(ARGS);
    b2Shape_EnableSensorEvents(shape_id, flag);
}

CK_DLL_SFUN(b2_Shape_AreSensorEventsEnabled)
{
    RETURN->v_int = b2Shape_AreSensorEventsEnabled(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_EnableContactEvents)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    int flag = GET_NEXT_INT(ARGS);
    b2Shape_EnableContactEvents(shape_id, flag);
}

CK_DLL_SFUN(b2_Shape_AreContactEventsEnabled)
{
    RETURN->v_int = b2Shape_AreContactEventsEnabled(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_EnablePreSolveEvents)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    int flag = GET_NEXT_INT(ARGS);
    b2Shape_EnablePreSolveEvents(shape_id, flag);
}

CK_DLL_SFUN(b2_Shape_ArePreSolveEventsEnabled)
{
    RETURN->v_int = b2Shape_ArePreSolveEventsEnabled(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_EnableHitEvents)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    int flag = GET_NEXT_INT(ARGS);
    b2Shape_EnableHitEvents(shape_id, flag);
}

CK_DLL_SFUN(b2_Shape_AreHitEventsEnabled)
{
    RETURN->v_int = b2Shape_AreHitEventsEnabled(GET_B2_ID(b2ShapeId, ARGS));
}

CK_DLL_SFUN(b2_Shape_TestPoint)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 point = GET_NEXT_VEC2(ARGS);
    RETURN->v_int  = b2Shape_TestPoint(shape_id, { (float)point.x, (float)point.y });
}

CK_DLL_SFUN(b2_Shape_RayCast)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 origin          = GET_NEXT_VEC2(ARGS);
    t_CKVEC2 translation     = GET_NEXT_VEC2(ARGS);
    Chuck_Object* output_obj = GET_NEXT_OBJECT(ARGS);
    b2CastOutput output
      = b2Shape_RayCast(shape_id, { (float)origin.x, (float)origin.y },
                        { (float)translation.x, (float)translation.y });
    b2CastOutput_to_ckobj(API, output_obj, &output);
    RETURN->v_object = output_obj;
}

CK_DLL_SFUN(b2_Shape_GetCircle)
{
    b2Circle circle          = b2Shape_GetCircle(GET_B2_ID(b2ShapeId, ARGS));
    Chuck_Object* circle_obj = chugin_createCkObj("b2_Circle", false, SHRED);
    b2Circle_to_ckobj(API, circle_obj, &circle);
    RETURN->v_object = circle_obj;
}

CK_DLL_SFUN(b2_Shape_GetSegment)
{
    b2Segment segment         = b2Shape_GetSegment(GET_B2_ID(b2ShapeId, ARGS));
    Chuck_Object* segment_obj = chugin_createCkObj("b2_Segment", false, SHRED);
    b2Segment_to_ckobj(API, segment_obj, &segment);
    RETURN->v_object = segment_obj;
}

// CK_DLL_SFUN(b2_Shape_GetSmoothSegment)
// {
//     b2SmoothSegment smooth_segment
//       = b2Shape_GetSmoothSegment(GET_B2_ID(b2ShapeId, ARGS));
//     Chuck_Object* smooth_segment_obj
//       = chugin_createCkObj("b2_SmoothSegment", false, SHRED);
//     b2SmoothSegment_to_ckobj(API, smooth_segment_obj, &smooth_segment);
//     RETURN->v_object = smooth_segment_obj;
// }

CK_DLL_SFUN(b2_Shape_GetCapsule)
{
    b2Capsule capsule         = b2Shape_GetCapsule(GET_B2_ID(b2ShapeId, ARGS));
    Chuck_Object* capsule_obj = chugin_createCkObj("b2_Capsule", false, SHRED);
    b2Capsule_to_ckobj(API, capsule_obj, &capsule);
    RETURN->v_object = capsule_obj;
}

CK_DLL_SFUN(b2_Shape_GetPolygon)
{
    b2Polygon polygon = b2Shape_GetPolygon(GET_B2_ID(b2ShapeId, ARGS));
    RETURN->v_object  = b2_polygon_create(SHRED, &polygon);
}

CK_DLL_SFUN(b2_Shape_SetCircle)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Circle circle = {};
    ckobj_to_b2Circle(API, &circle, GET_NEXT_OBJECT(ARGS));
    b2Shape_SetCircle(shape_id, &circle);
}

CK_DLL_SFUN(b2_Shape_SetCapsule)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Capsule capsule = {};
    ckobj_to_b2Capsule(API, &capsule, GET_NEXT_OBJECT(ARGS));
    b2Shape_SetCapsule(shape_id, &capsule);
}

CK_DLL_SFUN(b2_Shape_SetSegment)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Segment segment = {};
    ckobj_to_b2Segment(API, &segment, GET_NEXT_OBJECT(ARGS));
    b2Shape_SetSegment(shape_id, &segment);
}

CK_DLL_SFUN(b2_Shape_SetPolygon)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Polygon* polygon
      = OBJ_MEMBER_B2_PTR(b2Polygon, GET_NEXT_OBJECT(ARGS), b2_polygon_data_offset);
    b2Shape_SetPolygon(shape_id, polygon);
}

CK_DLL_SFUN(b2_Shape_GetParentChain)
{
    RETURN_B2_ID(b2ChainId, b2Shape_GetParentChain(GET_B2_ID(b2ShapeId, ARGS)));
}

CK_DLL_SFUN(b2_Shape_GetContactCapacity)
{
    RETURN->v_int = b2Shape_GetContactCapacity(GET_B2_ID(b2ShapeId, ARGS));
}

// CK_DLL_SFUN(b2_Shape_GetContactData)
// {
//     b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     Chuck_ArrayInt* contacts = GET_NEXT_OBJECT_ARRAY(ARGS);

//     // allocate mem in frame arena for contacts
//     int contact_capacity = b2Shape_GetContactCapacity(shape_id);
//     b2ContactData* contact_data
//       = ARENA_PUSH_COUNT(&audio_frame_arena, b2ContactData,
//       contact_capacity);

//     b2Shape_GetContactData(shape_id, contact_data, contact_capacity);

//     // copy contact data back to chuck array
//     // TODO
// }

CK_DLL_SFUN(b2_Shape_GetAABB)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    b2AABB aabb        = b2Shape_GetAABB(shape_id);
    RETURN->v_vec4
      = { aabb.lowerBound.x, aabb.lowerBound.y, aabb.upperBound.x, aabb.upperBound.y };
}

CK_DLL_SFUN(b2_Shape_GetClosestPoint)
{
    b2ShapeId shape_id = GET_B2_ID(b2ShapeId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 target = GET_NEXT_VEC2(ARGS);
    b2Vec2 closest_point
      = b2Shape_GetClosestPoint(shape_id, { (float)target.x, (float)target.y });
    RETURN->v_vec2 = { closest_point.x, closest_point.y };
}

// ============================================================================
// b2_BodyDef
// ============================================================================

static void b2BodyDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2BodyDef* obj)
{
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_type_offset) = obj->type;
    OBJ_MEMBER_VEC2(ckobj, b2_BodyDef_position_offset)
      = { obj->position.x, obj->position.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angle_offset) = obj->angle;
    OBJ_MEMBER_VEC2(ckobj, b2_BodyDef_linearVelocity_offset)
      = { obj->linearVelocity.x, obj->linearVelocity.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angularVelocity_offset) = obj->angularVelocity;
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_linearDamping_offset)   = obj->linearDamping;
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angularDamping_offset)  = obj->angularDamping;
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_gravityScale_offset)    = obj->gravityScale;
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_sleepThreshold_offset)  = obj->sleepThreshold;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_enableSleep_offset)       = obj->enableSleep;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_isAwake_offset)           = obj->isAwake;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_fixedRotation_offset)     = obj->fixedRotation;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_isBullet_offset)          = obj->isBullet;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_isEnabled_offset)         = obj->isEnabled;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_automaticMass_offset)     = obj->automaticMass;
}

static void ckobj_to_b2BodyDef(CK_DL_API API, b2BodyDef* obj, Chuck_Object* ckobj)
{
    obj->type              = (b2BodyType)OBJ_MEMBER_INT(ckobj, b2_BodyDef_type_offset);
    t_CKVEC2 position_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_BodyDef_position_offset);
    obj->position          = { (float)position_vec2.x, (float)position_vec2.y };
    obj->angle             = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angle_offset);
    t_CKVEC2 linearVelocity_vec2
      = OBJ_MEMBER_VEC2(ckobj, b2_BodyDef_linearVelocity_offset);
    obj->linearVelocity
      = { (float)linearVelocity_vec2.x, (float)linearVelocity_vec2.y };
    obj->angularVelocity
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angularVelocity_offset);
    obj->linearDamping
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_linearDamping_offset);
    obj->angularDamping
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angularDamping_offset);
    obj->gravityScale = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_gravityScale_offset);
    obj->sleepThreshold
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_sleepThreshold_offset);
    obj->enableSleep   = OBJ_MEMBER_INT(ckobj, b2_BodyDef_enableSleep_offset);
    obj->isAwake       = OBJ_MEMBER_INT(ckobj, b2_BodyDef_isAwake_offset);
    obj->fixedRotation = OBJ_MEMBER_INT(ckobj, b2_BodyDef_fixedRotation_offset);
    obj->isBullet      = OBJ_MEMBER_INT(ckobj, b2_BodyDef_isBullet_offset);
    obj->isEnabled     = OBJ_MEMBER_INT(ckobj, b2_BodyDef_isEnabled_offset);
    obj->automaticMass = OBJ_MEMBER_INT(ckobj, b2_BodyDef_automaticMass_offset);
}

CK_DLL_CTOR(b2_BodyDef_ctor)
{
    b2BodyDef default_body_def = b2DefaultBodyDef();
    b2BodyDef_to_ckobj(API, SELF, &default_body_def);
}

// ============================================================================
// b2_Body
// ============================================================================

CK_DLL_SFUN(b2_body_is_valid)
{
    RETURN->v_int = b2Body_IsValid(GET_B2_ID(b2BodyId, ARGS));
}

CK_DLL_SFUN(b2_body_get_type)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_GetType(body_id);
}

CK_DLL_SFUN(b2_body_set_type)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetType(body_id, ckint_to_b2BodyType(GET_NEXT_INT(ARGS)));
}

CK_DLL_SFUN(b2_body_get_position)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Vec2 pos     = b2Body_GetPosition(body_id);
    RETURN->v_vec2 = { pos.x, pos.y };
}

CK_DLL_SFUN(b2_body_get_rotation)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Rot rot         = b2Body_GetRotation(body_id);
    RETURN->v_complex = { rot.c, rot.s };
}

CK_DLL_SFUN(b2_body_get_angle)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetAngle(body_id);
}

CK_DLL_SFUN(b2_body_set_transform)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 pos = GET_NEXT_VEC2(ARGS);
    float angle  = GET_NEXT_FLOAT(ARGS);
    b2Body_SetTransform(body_id, { (float)pos.x, (float)pos.y }, angle);
}

CK_DLL_SFUN(b2_body_get_local_point)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 world_point = GET_NEXT_VEC2(ARGS);
    b2Vec2 local_point
      = b2Body_GetLocalPoint(body_id, { (float)world_point.x, (float)world_point.y });
    RETURN->v_vec2 = { local_point.x, local_point.y };
}

CK_DLL_SFUN(b2_body_get_world_point)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 local_point = GET_NEXT_VEC2(ARGS);
    b2Vec2 world_point
      = b2Body_GetWorldPoint(body_id, { (float)local_point.x, (float)local_point.y });
    RETURN->v_vec2 = { world_point.x, world_point.y };
}

CK_DLL_SFUN(b2_body_get_local_vector)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 world_vector = GET_NEXT_VEC2(ARGS);
    b2Vec2 local_vector   = b2Body_GetLocalVector(
      body_id, { (float)world_vector.x, (float)world_vector.y });
    RETURN->v_vec2 = { local_vector.x, local_vector.y };
}

CK_DLL_SFUN(b2_body_get_world_vector)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 local_vector = GET_NEXT_VEC2(ARGS);
    b2Vec2 world_vector   = b2Body_GetWorldVector(
      body_id, { (float)local_vector.x, (float)local_vector.y });
    RETURN->v_vec2 = { world_vector.x, world_vector.y };
}

CK_DLL_SFUN(b2_body_get_linear_velocity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Vec2 vel     = b2Body_GetLinearVelocity(body_id);
    RETURN->v_vec2 = { vel.x, vel.y };
}

CK_DLL_SFUN(b2_body_set_linear_velocity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 vel = GET_NEXT_VEC2(ARGS);
    b2Body_SetLinearVelocity(body_id, { (float)vel.x, (float)vel.y });
}

CK_DLL_SFUN(b2_body_get_angular_velocity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetAngularVelocity(body_id);
}

CK_DLL_SFUN(b2_body_set_angular_velocity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetAngularVelocity(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_apply_force)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 force = GET_NEXT_VEC2(ARGS);
    t_CKVEC2 point = GET_NEXT_VEC2(ARGS);
    t_CKINT wake   = GET_NEXT_INT(ARGS);
    b2Body_ApplyForce(body_id, { (float)force.x, (float)force.y },
                      { (float)point.x, (float)point.y }, wake);
}

CK_DLL_SFUN(b2_body_apply_force_to_center)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 force = GET_NEXT_VEC2(ARGS);
    t_CKINT wake   = GET_NEXT_INT(ARGS);
    b2Body_ApplyForceToCenter(body_id, { (float)force.x, (float)force.y }, wake);
}

CK_DLL_SFUN(b2_body_apply_torque)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKFLOAT torque = GET_NEXT_FLOAT(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyTorque(body_id, torque, wake);
}

CK_DLL_SFUN(b2_body_apply_linear_impulse)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 impulse = GET_NEXT_VEC2(ARGS);
    t_CKVEC2 point   = GET_NEXT_VEC2(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyLinearImpulse(body_id, { (float)impulse.x, (float)impulse.y },
                              { (float)point.x, (float)point.y }, wake);
}

CK_DLL_SFUN(b2_body_apply_linear_impulse_to_center)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 impulse = GET_NEXT_VEC2(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyLinearImpulseToCenter(body_id, { (float)impulse.x, (float)impulse.y },
                                      wake);
}

CK_DLL_SFUN(b2_body_apply_angular_impulse)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKFLOAT impulse = GET_NEXT_FLOAT(ARGS);
    t_CKINT wake      = GET_NEXT_INT(ARGS);
    b2Body_ApplyAngularImpulse(body_id, impulse, wake);
}

CK_DLL_SFUN(b2_body_get_mass)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetMass(body_id);
}

CK_DLL_SFUN(b2_body_get_inertia)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetInertiaTensor(body_id);
}

CK_DLL_SFUN(b2_body_get_local_center_of_mass)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Vec2 center  = b2Body_GetLocalCenterOfMass(body_id);
    RETURN->v_vec2 = { center.x, center.y };
}

CK_DLL_SFUN(b2_body_get_world_center_of_mass)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Vec2 center  = b2Body_GetWorldCenterOfMass(body_id);
    RETURN->v_vec2 = { center.x, center.y };
}

// CK_DLL_SFUN(b2_body_get_mass_data)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     b2MassData data           = b2Body_GetMassData(body_id);
//     Chuck_Object* mass_data   = chugin_createCkObj("b2_MassData", false,
//     SHRED); b2MassData* mass_data_ptr = new b2MassData;

//     OBJ_MEMBER_UINT(mass_data, b2_mass_data_offset) =
//     (t_CKUINT)mass_data_ptr;

//     // TODO: impl after Ge gets back on API->object->create() not invoking
//     // default ctor

//     memcpy(mass_data_ptr, &data, sizeof(data));

//     RETURN->v_object = mass_data;
// }

// CK_DLL_SFUN(b2_body_set_mass_data)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     Chuck_Object* mass_data_obj = GET_NEXT_OBJECT(ARGS);
//     b2MassData* mass_data
//       = (b2MassData*)OBJ_MEMBER_UINT(mass_data_obj, b2_mass_data_offset);
//     b2Body_SetMassData(body_id, *mass_data);
// }

CK_DLL_SFUN(b2_body_apply_mass_from_shapes)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_ApplyMassFromShapes(body_id);
}

CK_DLL_SFUN(b2_body_set_linear_damping)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetLinearDamping(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_get_linear_damping)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetLinearDamping(body_id);
}

CK_DLL_SFUN(b2_body_set_angular_damping)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetAngularDamping(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_get_angular_damping)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetAngularDamping(body_id);
}

CK_DLL_SFUN(b2_body_set_gravity_scale)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetGravityScale(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_get_gravity_scale)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetGravityScale(body_id);
}

CK_DLL_SFUN(b2_body_is_awake)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsAwake(body_id);
}

CK_DLL_SFUN(b2_body_set_awake)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetAwake(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_enable_sleep)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_EnableSleep(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_is_sleep_enabled)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsSleepEnabled(body_id);
}

CK_DLL_SFUN(b2_body_set_sleep_threshold)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetSleepThreshold(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_get_sleep_threshold)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetSleepThreshold(body_id);
}

CK_DLL_SFUN(b2_body_is_enabled)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsEnabled(body_id);
}

CK_DLL_SFUN(b2_body_disable)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_Disable(body_id);
}

CK_DLL_SFUN(b2_body_enable)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_Enable(body_id);
}

CK_DLL_SFUN(b2_body_set_fixed_rotation)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetFixedRotation(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_is_fixed_rotation)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsFixedRotation(body_id);
}

CK_DLL_SFUN(b2_body_set_bullet)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetBullet(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_is_bullet)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsBullet(body_id);
}

CK_DLL_SFUN(b2_body_enable_hit_events)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_EnableHitEvents(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_get_shape_count)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_GetShapeCount(body_id);
}

CK_DLL_SFUN(b2_body_get_shapes)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    Chuck_ArrayInt* ck_shape_array = GET_NEXT_OBJECT_ARRAY(ARGS);
    API->object->array_int_clear(ck_shape_array);

    // get b2ShapeIds
    int shape_count = b2Body_GetShapeCount(body_id);
    b2ShapeId* shape_id_array
      = ARENA_PUSH_COUNT(&audio_frame_arena, b2ShapeId, shape_count);
    b2Body_GetShapes(body_id, shape_id_array, shape_count);

    // copy into array
    for (int i = 0; i < shape_count; i++) {
        t_CKINT shape_id = *(t_CKINT*)(shape_id_array + i);
        API->object->array_int_push_back(ck_shape_array, shape_id);
    }
}

CK_DLL_SFUN(b2_body_get_joint_count)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_GetJointCount(body_id);
}

// CK_DLL_SFUN(b2_body_get_joints)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     // TODO
// }

CK_DLL_SFUN(b2_body_get_contact_capacity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_GetContactCapacity(body_id);
}

// CK_DLL_SFUN(b2_body_get_contact_data)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     // TODO
// }

CK_DLL_SFUN(b2_body_compute_aabb)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2AABB box = b2Body_ComputeAABB(body_id);
    RETURN->v_vec4
      = { box.lowerBound.x, box.lowerBound.y, box.upperBound.x, box.upperBound.y };
}

// ============================================================================
// b2_Circle
// ============================================================================

static void b2Circle_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Circle* obj)
{
    OBJ_MEMBER_VEC2(ckobj, b2_Circle_position_offset)
      = { obj->center.x, obj->center.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_Circle_radius_offset) = obj->radius;
}

static void ckobj_to_b2Circle(CK_DL_API API, b2Circle* obj, Chuck_Object* ckobj)
{
    t_CKVEC2 position_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_Circle_position_offset);
    obj->center            = { (float)position_vec2.x, (float)position_vec2.y };
    obj->radius            = (float)OBJ_MEMBER_FLOAT(ckobj, b2_Circle_radius_offset);
}

CK_DLL_CTOR(b2_Circle_ctor)
{
    OBJ_MEMBER_VEC2(SELF, b2_Circle_position_offset) = GET_NEXT_VEC2(ARGS);
    OBJ_MEMBER_FLOAT(SELF, b2_Circle_radius_offset)  = GET_NEXT_FLOAT(ARGS);
}

// ============================================================================
// b2_Capsule
// ============================================================================

static void b2Capsule_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Capsule* obj)
{
    OBJ_MEMBER_VEC2(ckobj, b2_Capsule_center1_offset)
      = { obj->center1.x, obj->center1.y };
    OBJ_MEMBER_VEC2(ckobj, b2_Capsule_center2_offset)
      = { obj->center2.x, obj->center2.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_Capsule_radius_offset) = obj->radius;
}

static void ckobj_to_b2Capsule(CK_DL_API API, b2Capsule* obj, Chuck_Object* ckobj)
{
    t_CKVEC2 center1_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_Capsule_center1_offset);
    obj->center1          = { (float)center1_vec2.x, (float)center1_vec2.y };
    t_CKVEC2 center2_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_Capsule_center2_offset);
    obj->center2          = { (float)center2_vec2.x, (float)center2_vec2.y };
    obj->radius           = (float)OBJ_MEMBER_FLOAT(ckobj, b2_Capsule_radius_offset);
}

CK_DLL_CTOR(b2_Capsule_ctor)
{
    OBJ_MEMBER_VEC2(SELF, b2_Capsule_center1_offset) = GET_NEXT_VEC2(ARGS);
    OBJ_MEMBER_VEC2(SELF, b2_Capsule_center2_offset) = GET_NEXT_VEC2(ARGS);
    OBJ_MEMBER_FLOAT(SELF, b2_Capsule_radius_offset) = GET_NEXT_FLOAT(ARGS);
}

// ============================================================================
// b2_Segment
// ============================================================================

static void b2Segment_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Segment* obj)
{
    OBJ_MEMBER_VEC2(ckobj, b2_Segment_point1_offset) = { obj->point1.x, obj->point1.y };
    OBJ_MEMBER_VEC2(ckobj, b2_Segment_point2_offset) = { obj->point2.x, obj->point2.y };
}

static void ckobj_to_b2Segment(CK_DL_API API, b2Segment* obj, Chuck_Object* ckobj)
{
    t_CKVEC2 point1_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_Segment_point1_offset);
    obj->point1          = { (float)point1_vec2.x, (float)point1_vec2.y };
    t_CKVEC2 point2_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_Segment_point2_offset);
    obj->point2          = { (float)point2_vec2.x, (float)point2_vec2.y };
}

CK_DLL_CTOR(b2_Segment_ctor)
{
    OBJ_MEMBER_VEC2(SELF, b2_Segment_point1_offset) = GET_NEXT_VEC2(ARGS);
    OBJ_MEMBER_VEC2(SELF, b2_Segment_point2_offset) = GET_NEXT_VEC2(ARGS);
}

// ============================================================================
// b2_CastOutput
// ============================================================================

static void b2CastOutput_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2CastOutput* obj)
{
    OBJ_MEMBER_VEC2(ckobj, b2_CastOutput_normal_offset)
      = { obj->normal.x, obj->normal.y };
    OBJ_MEMBER_VEC2(ckobj, b2_CastOutput_point_offset) = { obj->point.x, obj->point.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_CastOutput_fraction_offset) = obj->fraction;
    OBJ_MEMBER_INT(ckobj, b2_CastOutput_iterations_offset) = obj->iterations;
    OBJ_MEMBER_INT(ckobj, b2_CastOutput_hit_offset)        = obj->hit;
}

// static void ckobj_to_b2CastOutput(CK_DL_API API, b2CastOutput* obj,
// Chuck_Object* ckobj)
// {
// t_CKVEC2 normal_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_CastOutput_normal_offset);
// obj->normal = { (float) normal_vec2.x, (float) normal_vec2.y };
// t_CKVEC2 point_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_CastOutput_point_offset);
// obj->point = { (float) point_vec2.x, (float) point_vec2.y };
// obj->fraction = (float) OBJ_MEMBER_FLOAT(ckobj,
// b2_CastOutput_fraction_offset); obj->iterations = OBJ_MEMBER_INT(ckobj,
// b2_CastOutput_iterations_offset); obj->hit = OBJ_MEMBER_INT(ckobj,
// b2_CastOutput_hit_offset);
// }

// ============================================================================
// b2_ContactHitEvent
// ============================================================================

static void b2ContactHitEvent_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                       b2ContactHitEvent* obj)
{

    OBJ_MEMBER_B2_ID(b2ShapeId, ckobj, b2_ContactHitEvent_shapeIdA_offset)
      = obj->shapeIdA;
    OBJ_MEMBER_B2_ID(b2ShapeId, ckobj, b2_ContactHitEvent_shapeIdB_offset)
      = obj->shapeIdB;
    OBJ_MEMBER_VEC2(ckobj, b2_ContactHitEvent_point_offset)
      = { obj->point.x, obj->point.y };
    OBJ_MEMBER_VEC2(ckobj, b2_ContactHitEvent_normal_offset)
      = { obj->normal.x, obj->normal.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_ContactHitEvent_approachSpeed_offset)
      = obj->approachSpeed;
}

// static void ckobj_to_b2ContactHitEvent(CK_DL_API API, b2ContactHitEvent* obj,
// Chuck_Object* ckobj)
// {
// obj->shapeIdA = OBJ_MEMBER_INT(ckobj, b2_ContactHitEvent_shapeIdA_offset);
// obj->shapeIdB = OBJ_MEMBER_INT(ckobj, b2_ContactHitEvent_shapeIdB_offset);
// t_CKVEC2 point_vec2 = OBJ_MEMBER_VEC2(ckobj,
// b2_ContactHitEvent_point_offset); obj->point = { (float) point_vec2.x,
// (float) point_vec2.y }; t_CKVEC2 normal_vec2 = OBJ_MEMBER_VEC2(ckobj,
// b2_ContactHitEvent_normal_offset); obj->normal = { (float) normal_vec2.x,
// (float) normal_vec2.y }; obj->approachSpeed = (float) OBJ_MEMBER_FLOAT(ckobj,
// b2_ContactHitEvent_approachSpeed_offset);
// }

with Ada.Streams;
procedure GeneralFreezingErrors is
    -- Confirming representation items are illegal after freezing too.
    type Scalar is range 0 .. 100;
    Value : Scalar;
    for Scalar'Size use 32;
    for Scalar'Alignment use 4;
    for Scalar'Stream_Size use 32;

    type Indexed is range 1 .. 2;
    type Array_Type is array (Indexed) of Integer;
    for Indexed'Size use 32;
    for Array_Type'Size use 64;
    Data : Array_Type;
    for Array_Type'Size use 64;

    type Enum is (A, B);
    Width : constant := Enum'Width;
    for Enum use (0, 1);

    type Parent is tagged null record;
    type Captured is record
        Item : Integer;
    end record;
    procedure Primitive (Self : Parent; Item : Captured);
    Object : Parent;
    for Captured use record end record;
    procedure Too_Late (Self : Parent) is null;
    for Parent'Size use 64;
    procedure Primitive (Self : Parent; Item : Captured) is null;

    type Callback_Result is null record;
    function Build return Callback_Result;
    type Callback is access function return Callback_Result;
    F : Callback := null;
    for Callback_Result use record end record;
    for Callback'Size use 64;
    function Build return Callback_Result is
    begin
        return (null record);
    end Build;

    type Allocated is null record;
    type Pointer is access Allocated;
    function Defaulted (P : Pointer := new Allocated) return Integer;
    Early : Integer := Defaulted;
    for Allocated use record end record;
    function Defaulted (P : Pointer) return Integer is
    begin
        return 1;
    end Defaulted;

    type Generic_Source is null record;
    function Produce return Generic_Source;
    function Read (Item : Generic_Source) return Integer;
    generic
        X : Integer := Read (Produce);
    package G is end G;
    package Instance is new G;
    for Generic_Source use record end record;
    function Produce return Generic_Source is
    begin
        return (null record);
    end Produce;
    function Read (Item : Generic_Source) return Integer is
    begin
        return 1;
    end Read;

    type Static_Default is null record;
    function Static_Query (Size : Integer := Static_Default'Size) return Integer;
    for Static_Default use record end record;
    function Static_Query (Size : Integer) return Integer is
    begin
        return Size;
    end Static_Query;

    package Classwide_Profile is
        type Root is tagged null record;
        type Hook is access procedure (Self : Root'Class);
        Action : Hook := null;
        procedure Late (Self : Root) is null;
    end Classwide_Profile;

    package Imported_Primitive is
        type Root is tagged null record;
        procedure Act (Self : Root);
        Object : Root;
        pragma Import (Ada, Act, "late_primitive");
        procedure Act (Self : Root) is null;
    end Imported_Primitive;

    type Sized is range 0 .. 100;
    for Sized'Size use Sized'Size;

    type Shared is range 0 .. 100;
    Width_Of_Shared : constant := Shared'Width;
    pragma Atomic (Shared);

    type Function_Pointer is access function return Integer;
    Null_Function : Function_Pointer := null;
    pragma Convention (C, Function_Pointer);

    package Progenitor_Profile is
        type Dependency is null record;
        type Face is interface;
        procedure Use_Dependency (Self : Face; Item : Dependency) is abstract;
        type Concrete is new Face with null record;
        overriding procedure Use_Dependency (Self : Concrete; Item : Dependency) is null;
        for Dependency use record end record;
    end Progenitor_Profile;

    Shared_Object : Integer := 1;
    Snapshot : Integer := Shared_Object;
    pragma Atomic (Shared_Object);

    type Controlled_Link is access Integer;
    Link : Controlled_Link := null;
    pragma Controlled (Controlled_Link);

    type Captured_Parent is record
        Value : Integer;
    end record;
    subtype Parent_View is Captured_Parent;
    type Capture_Child is new Parent_View;
    for Parent_View'Size use 64;

    type Index_Base is range 1 .. 2;
    subtype Index_View is Index_Base;
    type Open_Index_Array is array (Index_Base range <>) of Integer;
    subtype Index_Array is Open_Index_Array (Index_View);
    Captured_Array : Index_Array;
    for Index_View'Size use 64;
begin
    null;
end GeneralFreezingErrors;

generic
    type Index is (<>);
    type Element is private;
    type Element_Array is array (Index range <>) of aliased Element;
    Default_Terminator : Element;
package Interfaces.C.Pointers is
    pragma Preelaborate (Pointers);

    type Pointer is access all Element;

    function Value (Ref : Pointer; Terminator : Element := Default_Terminator) return Element_Array;
    function Value (Ref : Pointer; Length : ptrdiff_t) return Element_Array;

    Pointer_Error : exception;

    function "+" (Left : Pointer; Right : ptrdiff_t) return Pointer;
    function "+" (Left : ptrdiff_t; Right : Pointer) return Pointer;
    function "-" (Left : Pointer; Right : ptrdiff_t) return Pointer;
    function "-" (Left, Right : Pointer) return ptrdiff_t;

    procedure Increment (Ref : in out Pointer);
    procedure Decrement (Ref : in out Pointer);

    pragma Convention (Intrinsic, "+");
    pragma Convention (Intrinsic, "-");
    pragma Convention (Intrinsic, Increment);
    pragma Convention (Intrinsic, Decrement);

    function Virtual_Length (Ref : Pointer; Terminator : Element := Default_Terminator) return ptrdiff_t;

    procedure Copy_Terminated_Array (Source, Target : Pointer; Limit : ptrdiff_t := ptrdiff_t'Last;
                                     Terminator : Element := Default_Terminator);
    procedure Copy_Array (Source, Target : Pointer; Length : ptrdiff_t);
end Interfaces.C.Pointers;

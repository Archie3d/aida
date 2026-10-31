procedure GenericDerivedErrors is
    package Model is
        type Root is tagged record
            Visible : Integer;
        end record;
        procedure Op (Object : Root) is null;
        type Child is new Root with record
            Extra : Integer;
        end record;
        type Abstract_Child is abstract new Root with null record;
        overriding procedure Op (Object : Abstract_Child) is abstract;
        type Rec (N : Positive) is null record;
        type Rec_Child is new Rec;
        subtype Rec_Pair is Rec (2);
        subtype Wrong_Rec is Rec_Child (3);
        type Fixed_Number is delta 0.25 range 0.0 .. 10.0;
        subtype Fixed_Narrow is Fixed_Number range 1.0 .. 2.0;
        type Fixed_Child is new Fixed_Number;
        type Number is range 0 .. 20;
        subtype Narrow is Number range 2 .. 5;
        type Number_Child is new Number;
        subtype Child_Narrow is Number_Child range 2 .. 4;
        type Vector is array (Integer range <>) of Integer;
        subtype Triple is Vector (1 .. 3);
        type Vector_Child is new Vector;
        subtype Same is Vector_Child (1 .. 3);
        subtype Shifted is Vector_Child (2 .. 4);
        type Other is tagged null record;
    end Model;
    generic
        type T is new Model.Root with private;
    package Tagged_Formal is
    end Tagged_Formal;
    generic
        type T is new Model.Narrow;
    package Narrow_Formal is
    end Narrow_Formal;
    generic
        type T is new Model.Triple;
    package Array_Formal is
    end Array_Formal;
    generic
        type T is new Model.Vector;
    package Unconstrained_Formal is
    end Unconstrained_Formal;
    generic
        type T is tagged private;
    package Private_Tagged is
    end Private_Tagged;
    generic
        type T is new Model.Fixed_Narrow;
    package Fixed_Formal is
    end Fixed_Formal;
    generic
        type T is new Model.Rec_Pair;
    package Record_Formal is
    end Record_Formal;
    package Record_Mismatch is new Record_Formal (Model.Wrong_Rec);
    package Fixed_Widening is new Fixed_Formal (Model.Fixed_Child);
    package Wrong_Ancestor is new Tagged_Formal (Model.Other);
    package Abstract_Actual is new Tagged_Formal (Model.Abstract_Child);
    package Wider_Range is new Narrow_Formal (Model.Number_Child);
    package Different_Bounds is new Array_Formal (Model.Shifted);
    package Constrained_Actual is new Unconstrained_Formal (Model.Same);
    package Scalar_Actual is new Private_Tagged (Integer);
    package Classwide_Actual is new Tagged_Formal (Model.Root'Class);
    generic
        type T is abstract new Model.Root with private;
    package Abstract_Formal is
    end Abstract_Formal;
    package Bad_Abstract_Operation is new Abstract_Formal (Model.Abstract_Child);
    package Good_Abstract_Formal is new Abstract_Formal (Model.Child);
    package Good_Tagged is new Tagged_Formal (Model.Child);
    package Good_Narrow is new Narrow_Formal (Model.Child_Narrow);
    package Good_Array is new Array_Formal (Model.Same);
    package Good_Unconstrained is new Unconstrained_Formal (Model.Vector_Child);
    generic
        type T is new Model.Root with private;
    function Hidden_Field (Object : T) return Integer;
    function Hidden_Field (Object : T) return Integer is
    begin
        return Object.Extra;
    end Hidden_Field;
    generic
        type T is abstract tagged private;
    procedure Abstract_Object;
    procedure Abstract_Object is
        Object : T;
    begin
        null;
    end Abstract_Object;
begin
    null;
end GenericDerivedErrors;

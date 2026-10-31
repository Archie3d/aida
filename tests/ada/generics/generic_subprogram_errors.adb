procedure Generic_Subprogram_Errors is
    generic
        type Item is private;
        with function Test (Left, Right : Item) return Boolean;
    procedure Check;
    procedure Check is
    begin
        null;
    end Check;
    function Wrong_Result (L, R : Integer) return Integer is
    begin
        return L;
    end Wrong_Result;
    function Wrong_Type (L, R : Float) return Boolean is
    begin
        return L < R;
    end Wrong_Type;
    function Wrong_Arity (L : Integer) return Boolean is
    begin
        return True;
    end Wrong_Arity;
    procedure Missing is new Check (Integer);
    procedure Result_Error is new Check (Integer, Wrong_Result);
    procedure Type_Error is new Check (Integer, Wrong_Type);
    procedure Arity_Error is new Check (Integer, Wrong_Arity);
    procedure Operator_Error is new Check (Integer, "+");
    procedure Duplicate is new Check (Integer, Item => Integer);
    procedure Positional is new Check (Item => Integer, "<");
    procedure Unknown is new Check (Integer, Missing => "<");

    generic
        with procedure Update (Value : in out Integer);
    procedure Change;
    procedure Change is
    begin
        null;
    end Change;
    procedure Read_Only (Value : Integer) is
    begin
        null;
    end Read_Only;
    procedure Bad_Mode is new Change (Read_Only);

    generic
        type Element is private;
        type Index_Type is (<>);
        type Array_Type is array (Index_Type range <>) of Element;
    procedure Array_Check;
    procedure Array_Check is
    begin
        null;
    end Array_Check;
    type Other_Index is (A, B);
    type Wrong_Index is array (Other_Index range <>) of Integer;
    type Wrong_Element is array (Integer range <>) of Float;
    type Wrong_Rank is array (Integer range <>, Integer range <>) of Integer;
    type Fixed is array (1 .. 3) of Integer;
    procedure Bad_Index is new Array_Check (Integer, Integer, Wrong_Index);
    procedure Bad_Element is new Array_Check (Integer, Integer, Wrong_Element);
    procedure Bad_Rank is new Array_Check (Integer, Integer, Wrong_Rank);
    procedure Bad_Constraint is new Array_Check (Integer, Integer, Fixed);
    procedure Not_Array is new Array_Check (Integer, Integer, Integer);

    generic
        with function Absent (Value : Integer) return Boolean is <>;
    procedure Default_Check;
    procedure Default_Check is
    begin
        null;
    end Default_Check;
    procedure Missing_Default is new Default_Check;

    type Different is new Integer;
    function Derived_Test (L, R : Different) return Boolean is
    begin
        return L < R;
    end Derived_Test;
    procedure Bad_Derived is new Check (Integer, Derived_Test);
    type Positive_Index is array (Positive range <>) of Integer;
    procedure Bad_Index_Subtype is new Array_Check (Integer, Integer, Positive_Index);
    type Positive_Element is array (Integer range <>) of Positive;
    procedure Bad_Element_Subtype is new Array_Check (Integer, Integer, Positive_Element);

    package One is
        function Test (L, R : Integer) return Boolean;
    end One;
    package Two is
        function Test (L, R : Integer) return Boolean;
    end Two;
    use One;
    use Two;
    procedure Ambiguous is new Check (Integer, Test);

    generic
        with function Test (L, R : Integer) return Boolean is Too_Late;
    procedure Early;
    procedure Early is
    begin
        null;
    end Early;
    function Too_Late (L, R : Integer) return Boolean is
    begin
        return L < R;
    end Too_Late;
    procedure Late_Default is new Early;
    subtype Short_Index is Integer range 1 .. 2;
    generic
        type Array_Type is array (Short_Index) of Integer;
    procedure Constrained;
    procedure Constrained is
    begin
        null;
    end Constrained;
    procedure Bad_Bounds is new Constrained (Fixed);
begin
    null;
end Generic_Subprogram_Errors;

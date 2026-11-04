procedure RecordFreezingErrors is
    type Element is null record;
    type Elements is array (1 .. 2) of Element;
    for Element use record end record;

    type Part is null record;
    type Container is record
        Value : Part;
    end record;
    for Part use record end record;

    type Parent is null record;
    type Child is new Parent;
    for Parent use record end record;

    type Observed is record
        Value : Integer;
    end record;
    Size : constant := Observed'Size;
    for Observed use record end record;

    type Allocated is null record;
    type Pointer is access Allocated;
    Object : Pointer := new Allocated;
    for Allocated use record end record;

    generic
        type Data is private;
    package Identity is end Identity;
    type Actual is null record;
    package Instance is new Identity (Actual);
    for Actual use record end record;

    package Public_Use is
        type Visible is null record;
        Object : Visible;
    private
        for Visible use record end record;
    end Public_Use;

    type After_Body is null record;
    procedure Boundary is
    begin
        null;
    end Boundary;
    for After_Body use record end record;

    type Late_Size is record
        Value : Integer;
    end record;
    Data : Late_Size;
    for Late_Size'Size use 64;
    for Late_Size'Alignment use 8;

    type Other is record
        Value : Integer;
    end record;
    type Target is record
        Value : Integer;
    end record;
    for Other'Size use Target'Size;
    for Target use record end record;

    type Default_Result is record
        Value : Integer;
    end record;
    function Build return Default_Result;
    function Score (Value : Default_Result) return Integer;
    function With_Default (Value : Integer := Score (Build)) return Integer;
    Before_Clause : Integer := With_Default;
    for Default_Result use record end record;
    function Build return Default_Result is
    begin
        return (Value => 1);
    end Build;
    function Score (Value : Default_Result) return Integer is
    begin
        return Value.Value;
    end Score;
    function With_Default (Value : Integer) return Integer is
    begin
        return Value;
    end With_Default;

    type Callback_Result is null record;
    function Create return Callback_Result;
    type Constructor is access function return Callback_Result;
    Callback : Constructor := Create'Access;
    for Callback_Result use record end record;
    function Create return Callback_Result is
    begin
        return (null record);
    end Create;

    type Profile_Item is null record;
    package Profile is
        procedure Consume (Item : Profile_Item) is null;
    end Profile;
    for Profile_Item use record end record;

    type Null_Result is null record;
    type Nullable_Callback is access function return Null_Result;
    Null_Callback : Nullable_Callback := null;
    for Null_Result use record end record;
begin
    null;
end RecordFreezingErrors;

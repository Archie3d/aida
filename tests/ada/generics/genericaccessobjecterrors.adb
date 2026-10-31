procedure GenericAccessObjectErrors is
    A : aliased Integer := 1;
    type Pointer is access all Integer;
    P : Pointer := A'Access;
    Const : constant Pointer := P;
    AP : access Integer := A'Access;
    CP : access constant Integer := A'Access;
    generic
        Value : in out Pointer;
    package Named is end Named;
    package Bad_Constant is new Named (Const);
    package Bad_Null is new Named (null);
    generic
        Value : access constant Integer;
    package Constant_View is end Constant_View;
    package Bad_View is new Constant_View (AP);
    generic
        Value : access Integer;
    package Variable_View is end Variable_View;
    package Bad_Const_View is new Variable_View (CP);
    generic
        Value : in out access Integer;
    package Anonymous_View is end Anonymous_View;
    package Bad_Named is new Anonymous_View (P);
    package Bad_Anon_Null is new Anonymous_View (null);
    generic
        Value : in out not null Pointer;
    package Excluding is end Excluding;
    package Bad_Null_Subtype is new Excluding (P);
    generic
        Value : in out not null access Integer;
    package Anonymous_Excluding is end Anonymous_Excluding;
    package Bad_Anon_Subtype is new Anonymous_Excluding (AP);
    subtype Small is Integer range 1 .. 3;
    type Small_Pointer is access all Small;
    S : Small_Pointer := new Small'(2);
    package Bad_Designated is new Variable_View (S);
    type Callback is access function (X : Small) return Small;
    function Next (X : Small) return Small is
    begin
        return X;
    end Next;
    CB : Callback := Next'Access;
    generic
        Value : access function (X : Integer) return Integer;
    package Callbacks is end Callbacks;
    package Bad_Profile is new Callbacks (CB);
    generic
        Value : access constant Integer := AP;
    package Bad_Default is end Bad_Default;
    Saved : access Integer;
    procedure Check_Escape is
        generic
            Value : access Integer;
        package Escaping is
            procedure Save;
        end Escaping;
        package body Escaping is
            procedure Save is
            begin
                Saved := Value;
            end Save;
        end Escaping;
    begin
        null;
    end Check_Escape;
    subtype NN is not null Pointer;
    generic
        Value : in out NN;
    package Outer is
        procedure Run;
    end Outer;
    package body Outer is
        procedure Run is
            package Bad_Forward is new Excluding (Value);
        begin
            null;
        end Run;
    end Outer;
begin
    null;
end GenericAccessObjectErrors;

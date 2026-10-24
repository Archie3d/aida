with Ada.Text_IO; use Ada.Text_IO;
procedure NullExclusions is
    type Link is access all Integer;
    subtype Required_Link is not null Link;
    type Required_Type is not null access all Integer;
    type Derived_Link is new Required_Link;
    type Callback is access function return Integer;
    subtype Required_Callback is not null Callback;
    Item : aliased Integer := 42;
    P : Required_Link := Item'Access;
    A : not null access Integer := Item'Access;
    C : not null access constant Integer := Item'Access;
    Calls : Integer := 0;
    Failures : Integer := 0;
    function Missing return Link is
    begin
        Calls := Calls + 1;
        return null;
    end Missing;
    procedure Take (Value : not null Link) is
    begin
        if Value.all /= 42 then raise Program_Error; end if;
    end Take;
    procedure Take_Anonymous (Value : not null access Integer) is
    begin
        if Value.all /= 42 then raise Program_Error; end if;
    end Take_Anonymous;
    procedure Set (Value : out Required_Link) is
    begin
        Value := Item'Access;
    end Set;
    procedure Clear (Value : out Link) is
    begin
        Value := null;
    end Clear;
    function Result return Required_Link is
    begin
        return Missing;
    end Result;
    function Anonymous_Result return not null access Integer is
    begin
        return Missing;
    end Anonymous_Result;
    function Extended (Value : Link) return Required_Link is
    begin
        return Object : Required_Link := P do
            Object := Value;
        end return;
    end Extended;
    procedure Defaulted (Value : not null access Integer := Missing) is
    begin
        raise Program_Error;
    end Defaulted;
    function Number return Integer is
    begin
        return 42;
    end Number;
    type Record_Value is record
        Value : Required_Link;
    end record;
    type Array_Value is array (1 .. 2) of Required_Link;
    type Holder is access Required_Link;
    B : Required_Callback := Number'Access;
    R : not null Link renames P;
    generic
        with procedure Action (Value : not null Link);
    procedure Apply;
    procedure Apply is
    begin
        Action (P);
    end Apply;
    procedure Apply_Take is new Apply (Take);
    procedure Alias (Value : not null Link) renames Take;
    generic
        with procedure Action (Value : Required_Link);
    procedure Named_Formal;
    procedure Named_Formal is
    begin
        Action (null);
    end Named_Formal;
    procedure Nullable (Value : Link) is
    begin
        if Value /= null then raise Program_Error; end if;
    end Nullable;
    procedure Nullable_Instance is new Named_Formal (Nullable);
    procedure Required_Instance is new Named_Formal (Take);
    generic
        with procedure Action (Value : not null access Integer);
    procedure Anonymous_Formal;
    procedure Anonymous_Formal is
    begin
        Action (P);
    end Anonymous_Formal;
    procedure Anonymous_Instance is new Anonymous_Formal (Take_Anonymous);
    protected Counter is
        function Read return Integer;
    end Counter;
    protected body Counter is
        function Read return Integer is
        begin
            return 42;
        end Read;
    end Counter;
    Protected_Callback : not null access protected function return Integer := Counter.Read'Access;
begin
    Take (P);
    Take_Anonymous (A);
    Alias (P);
    Apply_Take;
    if Protected_Callback.all /= 42 then raise Program_Error; end if;
    Nullable_Instance;
    Anonymous_Instance;
    if R.all /= 42 or C.all /= 42 or B.all /= 42 or P = null then raise Program_Error; end if;
    declare
        Nullable : Link;
    begin
        Set (Nullable);
        if Nullable.all /= 42 then raise Program_Error; end if;
    end;
    begin
        declare X : Required_Link; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Required_Type; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Derived_Link; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : not null access Integer; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Required_Link := Missing; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        P := Missing;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        A := Missing;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        Take (Missing);
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        Take_Anonymous (Missing);
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Link := Required_Link (Missing); begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Link := Required_Link'(Missing); begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Link := Result; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : access Integer := Anonymous_Result; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Record_Value; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Array_Value; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Array_Value := (P, Missing); begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Holder := new Required_Link; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Required_Callback; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        Clear (P);
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Link := Extended (Missing); begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        Defaulted;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    if Extended (P).all /= 42 then raise Program_Error; end if;
    begin
        Required_Instance;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : Holder := new Link; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    begin
        declare X : not null access protected function return Integer; begin null; end;
        raise Program_Error;
    exception
        when Constraint_Error => Failures := Failures + 1;
    end;
    if Failures /= 24 or Calls /= 12 or P.all /= 42 then raise Program_Error; end if;
    Put_Line ("null exclusions passed");
end NullExclusions;

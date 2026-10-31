with Ada.Text_IO; use Ada.Text_IO;
procedure InheritedCallbacks is
    Bias : Integer := 10;
    package Parent is
        type Root is tagged record
            Value : Integer := 0;
        end record;
        function Read (Object : Root) return Integer;
        procedure Change (Object : in out Root; Value : Integer);
        function Create (Value : Integer) return Root;
        function Through_Access (Object : access Root) return Integer;
    end Parent;
    package Child is
        type Derived is new Parent.Root with null record;
    end Child;
    type Reader is access function (Object : Child.Derived) return Integer;
    type Setter is access procedure (Object : in out Child.Derived; Value : Integer);
    type Constructor is access function (Value : Integer) return Child.Derived;
    type Access_Reader is access function (Object : not null access Child.Derived) return Integer;
    F : Reader := Child.Read'Access;
    Set : Setter := Child.Change'Access;
    Make : Constructor := Child.Create'Access;
    A : Access_Reader := Child.Through_Access'Access;
    X : aliased Child.Derived;
    function Alias (Object : Child.Derived) return Integer renames Child.Read;
    G : Reader := Alias'Access;
    function Before_Body return Boolean is
        Value : Integer;
    begin
        Value := F (X);
        return False;
    exception
        when Program_Error => return True;
    end Before_Body;
    Early_Check : Boolean := Before_Body;
    package body Parent is
        function Read (Object : Root) return Integer is
        begin
            return Object.Value + Bias;
        end Read;
        procedure Change (Object : in out Root; Value : Integer) is
        begin
            Object.Value := Value;
        end Change;
        function Create (Value : Integer) return Root is
        begin
            return (Value => Value);
        end Create;
        function Through_Access (Object : access Root) return Integer is
        begin
            return Object.Value + Bias;
        end Through_Access;
    end Parent;
begin
    if not Early_Check then raise Program_Error; end if;
    X := Make (32);
    if F (X) /= 42 or G (X) /= 42 or A (X'Access) /= 42 then raise Program_Error; end if;
    Set (X, 33);
    if F (X) /= 43 or F /= G or F /= Child.Read'Access then raise Program_Error; end if;
    if X'Tag /= Child.Derived'Tag then raise Program_Error; end if;
    Bias := 20;
    if F (X) /= 53 then raise Program_Error; end if;
    Put_Line ("inherited callbacks passed");
end InheritedCallbacks;

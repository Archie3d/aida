with Ada.Text_IO;
procedure GenericInterfaceNested is
    package Model is
        type A is interface;
        function Read (X : A) return Integer is abstract;
        type B is interface and A;
        type C is interface and A;
        type Diamond is interface and C and B;
        type Item is new Diamond with null record;
        overriding function Read (X : Item) return Integer;
        type Empty is interface;
        type Null_Only is interface;
        procedure Ping (X : Null_Only) is null;
    end Model;
    package body Model is
        function Read (X : Item) return Integer is
        begin
            return 31;
        end Read;
    end Model;
    generic
        type F is interface and Model.A;
    package Outer is
        generic
            type G is interface and F;
            type T is new G with private;
        function Score (X : T) return Integer;
    end Outer;
    package body Outer is
        function Score (X : T) return Integer is
        begin
            return Read (X) + Read (G'Class (X));
        end Score;
    end Outer;
    package First is new Outer (Model.Diamond);
    package Second is new Outer (Model.A);
    function First_Read is new First.Score (Model.Diamond, Model.Item);
    function Second_Read is new Second.Score (Model.B, Model.Item);
    generic
        type F is interface;
    package Builder is
        type Concrete is new F with null record;
    end Builder;
    package Empty_Builder is new Builder (Model.Empty);
    package Null_Builder is new Builder (Model.Null_Only);
    procedure Check is
        X : Model.Item;
        Empty : Empty_Builder.Concrete;
        Null_Value : Null_Builder.Concrete;
    begin
        Null_Builder.Ping (Null_Value);
        if First_Read (X) /= 62 or Second_Read (X) /= 62 then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("nested generic interface contracts passed");
end GenericInterfaceNested;

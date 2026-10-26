with Ada.Text_IO;
with Ada.Tags;
procedure GenericInterfaceFormals is
    use type Ada.Tags.Tag;
    package Model is
        type A is interface;
        function Read (X : A) return Integer is abstract;
        procedure Ping (X : A) is null;
        type B is interface;
        function Code (X : B) return Integer is abstract;
        type Both is interface and B and A;
        type Root is tagged record
            Value : Integer := 7;
        end record;
        function Noise (X : Root) return Integer;
        function Code (X : Root) return Integer;
        type Item is new Root and Both with null record;
        overriding function Read (X : Item) return Integer;
    end Model;
    package body Model is
        function Noise (X : Root) return Integer is
        begin
            return -1;
        end Noise;
        function Code (X : Root) return Integer is
        begin
            return X.Value + 20;
        end Code;
        function Read (X : Item) return Integer is
        begin
            return X.Value + 100;
        end Read;
    end Model;
    generic
        type F is interface and Model.A;
        type T is new F with private;
    package Views is
        function Static_Read (X : T) return Integer;
        function Dynamic_Read (X : F'Class) return Integer;
        function Identity return Ada.Tags.Tag;
        type Callback is access function (X : T) return Integer;
        function Callback_Read (X : T) return Integer;
    end Views;
    package body Views is
        function Static_Read (X : T) return Integer is
        begin
            Ping (X);
            return Read (X);
        end Static_Read;
        function Dynamic_Read (X : F'Class) return Integer is
        begin
            Ping (X);
            return Read (X);
        end Dynamic_Read;
        function Identity return Ada.Tags.Tag is
        begin
            return F'Tag;
        end Identity;
        function Callback_Read (X : T) return Integer is
            Call : Callback := Read'Access;
        begin
            return Call (X);
        end Callback_Read;
    end Views;
    package Concrete is new Views (Model.Both, Model.Item);
    generic
        type T is new Model.A with private;
    function Read_Implementation (X : T) return Integer;
    function Read_Implementation (X : T) return Integer is
    begin
        return Read (X);
    end Read_Implementation;
    function Read_Item is new Read_Implementation (Model.Item);
    generic
        type T is new Model.Root and Model.A with private;
    function Mixed_Read (X : T) return Integer;
    function Mixed_Read (X : T) return Integer is
    begin
        return Read (X) + Code (X);
    end Mixed_Read;
    function Mixed_Item is new Mixed_Read (Model.Item);
    procedure Check is
        X : Model.Item;
    begin
        if Concrete.Static_Read (X) /= 107 or Concrete.Dynamic_Read (X) /= 107
            or Concrete.Callback_Read (X) /= 107 or Read_Item (X) /= 107 or Mixed_Item (X) /= 134
            or Concrete.Identity /= Model.Both'Tag then
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
    Ada.Text_IO.Put_Line ("generic interface formals passed");
end GenericInterfaceFormals;

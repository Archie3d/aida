with Ada.Text_IO;
with Ada.Tags;
procedure GenericInterfaceCategories is
    use type Ada.Tags.Tag;
    type Ordinary is interface;
    type Limited_Contract is limited interface;
    type Synchronized_Contract is synchronized interface;
    type Task_Contract is task interface;
    type Protected_Contract is protected interface;
    type More_Task is task interface and Synchronized_Contract and Task_Contract;
    generic
        type T is interface;
    function Ordinary_Tag return Ada.Tags.Tag;
    function Ordinary_Tag return Ada.Tags.Tag is
    begin
        return T'Tag;
    end Ordinary_Tag;
    generic
        type T is limited interface;
    function Limited_Tag return Ada.Tags.Tag;
    function Limited_Tag return Ada.Tags.Tag is
    begin
        return T'Tag;
    end Limited_Tag;
    generic
        type T is synchronized interface;
    function Synchronized_Tag return Ada.Tags.Tag;
    function Synchronized_Tag return Ada.Tags.Tag is
    begin
        return T'Tag;
    end Synchronized_Tag;
    generic
        type T is task interface and Synchronized_Contract;
    function Task_Tag return Ada.Tags.Tag;
    function Task_Tag return Ada.Tags.Tag is
    begin
        return T'Tag;
    end Task_Tag;
    generic
        type T is protected interface;
    function Protected_Tag return Ada.Tags.Tag;
    function Protected_Tag return Ada.Tags.Tag is
    begin
        return T'Tag;
    end Protected_Tag;
    function Get_Ordinary is new Ordinary_Tag (Ordinary);
    function Get_Limited is new Limited_Tag (Limited_Contract);
    function Get_Synchronized is new Synchronized_Tag (Synchronized_Contract);
    function Get_Task is new Task_Tag (More_Task);
    function Get_Protected is new Protected_Tag (Protected_Contract);
    package Model is
        type I is limited interface;
        function Read (X : I) return Integer is abstract;
        type Nonlimited_Item is new I with null record;
        overriding function Read (X : Nonlimited_Item) return Integer;
        type Limited_Item is limited new I with null record;
        overriding function Read (X : Limited_Item) return Integer;
    end Model;
    package body Model is
        function Read (X : Nonlimited_Item) return Integer is
        begin
            return 17;
        end Read;
        function Read (X : Limited_Item) return Integer is
        begin
            return 29;
        end Read;
    end Model;
    generic
        type T is new Model.I with private;
    function Read_Nonlimited (X : T) return Integer;
    function Read_Nonlimited (X : T) return Integer is
    begin
        return Read (X);
    end Read_Nonlimited;
    generic
        type T is limited new Model.I with private;
    function Read_Limited (X : T) return Integer;
    function Read_Limited (X : T) return Integer is
    begin
        return Read (X);
    end Read_Limited;
    function Nonlimited_Read is new Read_Nonlimited (Model.Nonlimited_Item);
    function Limited_Read is new Read_Limited (Model.Limited_Item);
    function Limited_View_Of_Nonlimited is new Read_Limited (Model.Nonlimited_Item);
    procedure Check is
        A : Model.Nonlimited_Item;
        B : Model.Limited_Item;
    begin
        if Get_Ordinary /= Ordinary'Tag or Get_Limited /= Limited_Contract'Tag
            or Get_Synchronized /= Synchronized_Contract'Tag or Get_Task /= More_Task'Tag
            or Get_Protected /= Protected_Contract'Tag
            or Nonlimited_Read (A) /= 17 or Limited_Read (B) /= 29
            or Limited_View_Of_Nonlimited (A) /= 17 then
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
    Ada.Text_IO.Put_Line ("generic interface categories passed");
end GenericInterfaceCategories;

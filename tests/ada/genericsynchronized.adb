with Ada.Text_IO;
with Ada.Tags;
procedure GenericSynchronized is
    use type Ada.Tags.Tag;
    package Model is
        type Base is limited interface;
        procedure Ping (X : Base) is null;
        function Read (X : Base) return Integer is abstract;
        type Sync is synchronized interface and Base;
        type Task_View is task interface and Sync;
        type Protected_View is protected interface and Sync;
    end Model;
    generic
        type T is abstract synchronized new Model.Base with private;
    package View is
        type Callback is access procedure (X : T);
        function Has_Callback return Boolean;
        function Identity return Ada.Tags.Tag;
        function Read_Through (X : T'Class) return Integer;
        procedure Ping_Through (X : T'Class);
    end View;
    package body View is
        function Has_Callback return Boolean is
            P : Callback := Ping'Access;
        begin
            return P /= null;
        end Has_Callback;
        function Identity return Ada.Tags.Tag is
        begin
            return T'Tag;
        end Identity;
        function Read_Through (X : T'Class) return Integer is
        begin
            return Read (X);
        end Read_Through;
        procedure Ping_Through (X : T'Class) is
        begin
            Ping (X);
        end Ping_Through;
    end View;
    package Sync_View is new View (Model.Sync);
    package Task_View is new View (Model.Task_View);
    package Protected_View is new View (Model.Protected_View);
    generic
        type Ancestor is abstract tagged limited private;
        type T is abstract synchronized new Ancestor with private;
    function Parent_Identity return Ada.Tags.Tag;
    function Parent_Identity return Ada.Tags.Tag is
    begin
        return T'Tag;
    end Parent_Identity;
    function Sync_Parent is new Parent_Identity (Model.Sync, Model.Task_View);
    function Task_Parent is new Parent_Identity (Model.Task_View, Model.Task_View);
    function Protected_Parent is new Parent_Identity (Model.Protected_View, Model.Protected_View);
    procedure Check is
    begin
        if Sync_View.Identity /= Model.Sync'Tag
            or Task_View.Identity /= Model.Task_View'Tag
            or Protected_View.Identity /= Model.Protected_View'Tag
            or not Sync_View.Has_Callback or not Task_View.Has_Callback
            or not Protected_View.Has_Callback
            or Sync_Parent /= Model.Task_View'Tag or Task_Parent /= Model.Task_View'Tag
            or Protected_Parent /= Model.Protected_View'Tag then
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
    Ada.Text_IO.Put_Line ("synchronized generic formals passed");
end GenericSynchronized;

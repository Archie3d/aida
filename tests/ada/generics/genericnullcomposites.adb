with Ada.Text_IO;
procedure GenericNullComposites is
    type Limited_Record is limited record
        Value : Integer := 7;
    end record;
    generic
        type T (<>) is limited private;
        with procedure Visit (Object : T) is null;
    procedure Observe (Object : T);
    procedure Observe (Object : T) is
    begin
        Visit (Object);
    end Observe;
    procedure Observe_Limited is new Observe (Limited_Record);
    procedure Observe_String is new Observe (String);
    package Model is
        type Root is tagged record
            Value : Integer := 8;
        end record;
        procedure Reset (Object : out Root) is null;
    end Model;
    generic
        type T is new Model.Root with private;
        with procedure Reset (Object : out T) is null;
    procedure Leave_Alone (Object : out T);
    procedure Leave_Alone (Object : out T) is
    begin
        Reset (Object);
    end Leave_Alone;
    procedure Null_Reset is new Leave_Alone (Model.Root);
    type Link is access all Integer;
    generic
        with procedure Visit (Pointer : not null access Integer) is null;
    procedure Visit_Pointer (Pointer : access Integer);
    procedure Visit_Pointer (Pointer : access Integer) is
    begin
        Visit (Pointer);
    end Visit_Pointer;
    procedure Null_Visit is new Visit_Pointer;
    procedure Check is
        Limited_Object : Limited_Record;
        Object : Model.Root;
        Text : String (2 .. 5) := "test";
        Value : aliased Integer := 9;
        Missing : Link := null;
        Caught : Boolean := False;
    begin
        Observe_Limited (Limited_Object);
        Observe_String (Text (3 .. 4));
        Null_Reset (Object);
        Null_Visit (Value'Access);
        begin
            Null_Visit (Missing);
        exception
            when Constraint_Error => Caught := True;
        end;
        if Limited_Object.Value /= 7 or Object.Value /= 8 or Text /= "test"
            or Value /= 9 or not Caught then
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
    Ada.Text_IO.Put_Line ("composite null procedure defaults passed");
end GenericNullComposites;

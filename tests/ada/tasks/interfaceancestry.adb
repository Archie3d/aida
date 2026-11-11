with Ada.Text_IO;
with Ada.Tags;
procedure Interfaceancestry is
    use type Ada.Tags.Tag;
    package Model is
        type Ordinary is interface;
        procedure Touch (Self : in out Ordinary) is null;
        type Limited_Base is limited interface;
        type Sync is synchronized interface and Limited_Base;
        type Task_Base is task interface and Sync;
        type Protected_Base is protected interface and Sync;
        type Ordinary_Child is interface and Ordinary;
        type Limited_Child is limited interface and Limited_Base;
        type Sync_Child is synchronized interface and Sync and Limited_Base;
        type Task_Child is task interface and Task_Base and Sync;
        type Protected_Child is protected interface and Protected_Base and Sync;
        type Record_Child is new Ordinary_Child with record
            Value : Integer := 1;
        end record;
        type Limited_Record is limited new Limited_Base with record
            Value : Integer := 2;
        end record;
        type Private_Record is new Ordinary_Child with private;
        type Private_Task is synchronized new Task_Child and Limited_Base with private;
        type Private_Protected is synchronized new Protected_Child and Limited_Base with private;
        type Sync_Task is synchronized new Sync_Child with private;
        type Sync_Protected is synchronized new Sync_Child with private;
        procedure Read (Self : in out Private_Task; Value : out Integer);
        procedure Read (Self : in out Private_Protected; Value : out Integer);
        procedure Read (Self : in out Sync_Task; Value : out Integer);
        procedure Read (Self : in out Sync_Protected; Value : out Integer);
    private
        type Private_Record is new Ordinary_Child with null record;
        task type Private_Task is new Task_Child and Limited_Base with
            entry Ping (Value : out Integer);
        end Private_Task;
        protected type Private_Protected is new Protected_Child and Limited_Base with
            procedure Ping (Value : out Integer);
        end Private_Protected;
        task type Sync_Task is new Sync_Child with
            entry Ping (Value : out Integer);
        end Sync_Task;
        protected type Sync_Protected is new Sync_Child with
            procedure Ping (Value : out Integer);
        end Sync_Protected;
    end Model;
    package body Model is
        task body Private_Task is
        begin
            accept Ping (Value : out Integer) do Value := 3; end Ping;
        end Private_Task;
        protected body Private_Protected is
            procedure Ping (Value : out Integer) is
            begin
                Value := 4;
            end Ping;
        end Private_Protected;
        task body Sync_Task is
        begin
            accept Ping (Value : out Integer) do Value := 5; end Ping;
        end Sync_Task;
        protected body Sync_Protected is
            procedure Ping (Value : out Integer) is
            begin
                Value := 6;
            end Ping;
        end Sync_Protected;
        procedure Read (Self : in out Private_Task; Value : out Integer) is
        begin
            Self.Ping (Value);
        end Read;
        procedure Read (Self : in out Private_Protected; Value : out Integer) is
        begin
            Self.Ping (Value);
        end Read;
        procedure Read (Self : in out Sync_Task; Value : out Integer) is
        begin
            Self.Ping (Value);
        end Read;
        procedure Read (Self : in out Sync_Protected; Value : out Integer) is
        begin
            Self.Ping (Value);
        end Read;
    end Model;
    generic
        type O is interface and Model.Ordinary;
        type L is limited interface and Model.Limited_Base;
        type S is synchronized interface and Model.Sync;
        type T is task interface and Model.Task_Base;
        type P is protected interface and Model.Protected_Base;
        type OD is new Model.Ordinary with private;
        type LD is limited new Model.Limited_Base with private;
        type SD is synchronized new Model.Sync with private;
        type TD is synchronized new Model.Task_Base with private;
        type PD is synchronized new Model.Protected_Base with private;
    package Contracts is
        function Matches return Boolean;
    end Contracts;
    package body Contracts is
        function Matches return Boolean is
        begin
            return O'Tag = Model.Ordinary_Child'Tag
                and L'Tag = Model.Limited_Child'Tag
                and S'Tag = Model.Sync_Child'Tag
                and T'Tag = Model.Task_Child'Tag
                and P'Tag = Model.Protected_Child'Tag
                and OD'Tag = Model.Record_Child'Tag
                and LD'Tag = Model.Limited_Record'Tag
                and SD'Tag = Model.Sync_Protected'Tag
                and TD'Tag = Model.Private_Task'Tag
                and PD'Tag = Model.Private_Protected'Tag;
        end Matches;
    end Contracts;
    package Instance is new Contracts (Model.Ordinary_Child, Model.Limited_Child,
        Model.Sync_Child, Model.Task_Child, Model.Protected_Child, Model.Record_Child,
        Model.Limited_Record, Model.Sync_Protected, Model.Private_Task, Model.Private_Protected);
    R : Model.Record_Child;
    PR : Model.Private_Record;
    LR : Model.Limited_Record;
    T : Model.Private_Task;
    P : Model.Private_Protected;
    ST : Model.Sync_Task;
    SP : Model.Sync_Protected;
    Task_Value, Protected_Value, Sync_Task_Value, Sync_Protected_Value : Integer := 0;
begin
    -- Complete the rendezvous before checking values, so a failing assertion
    -- does not leave a task waiting for a caller.
    Model.Read (T, Task_Value);
    Model.Read (ST, Sync_Task_Value);
    Model.Read (P, Protected_Value);
    Model.Read (SP, Sync_Protected_Value);
    Model.Touch (R);
    Model.Touch (PR);
    if not Instance.Matches or else R.Value /= 1 or else LR.Value /= 2
        or else Task_Value /= 3 or else Protected_Value /= 4
        or else Sync_Task_Value /= 5 or else Sync_Protected_Value /= 6
        or else not (R in Model.Ordinary'Class) or else not (PR in Model.Ordinary'Class)
        or else not (LR in Model.Limited_Base'Class)
        or else not (T in Model.Task_Base'Class) or else not (T in Model.Sync'Class)
        or else not (P in Model.Protected_Base'Class) or else not (P in Model.Sync'Class)
        or else not (ST in Model.Sync'Class) or else not (SP in Model.Sync'Class) then
        raise Program_Error with "legal interface ancestry";
    end if;
    Ada.Text_IO.Put_Line ("interface ancestry passed");
end Interfaceancestry;

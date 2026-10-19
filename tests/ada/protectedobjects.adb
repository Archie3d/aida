with Ada.Text_IO;
with Ada.Exceptions;
with Ada.Unchecked_Deallocation;
procedure ProtectedObjects is
    procedure Check (condition : Boolean) is
    begin
        if not condition then
            raise Program_Error with "protected check failed";
        end if;
    end Check;
    protected type Counter is
        procedure Add (amount : Integer := 1);
        procedure Fail;
        procedure SetResult (result : out Integer);
        procedure Reenter;
        procedure Spawn;
        function Get return Integer;
        function Get (extra : Integer) return Integer;
        function Snapshot return String;
    private
        value : Integer := 0;
        text : String (1 .. 3) := "abc";
        procedure Bump;
        function Current return Integer;
    end Counter;
    subtype CounterAlias is Counter;
    earlyObject : Counter;
    function EarlyCall return Integer is
    begin
        return earlyObject.Get;
    exception
        when Program_Error => return -1;
    end EarlyCall;
    earlyResult : Integer := EarlyCall;
    protected body Counter is
        procedure Bump is
        begin
            value := value + 1;
        end Bump;
        function Current return Integer is
        begin
            return Counter.value;
        end Current;
        procedure Add (amount : Integer := 1) is
            procedure Nested is
            begin
                value := value + amount;
            end Nested;
        begin
            Nested;
            Counter.Bump;
        end Add;
        procedure Fail is
        begin
            Bump;
            raise Constraint_Error with "protected failure";
        end Fail;
        procedure SetResult (result : out Integer) is
        begin
            result := 99;
        end SetResult;
        procedure Reenter is
            aliasInstance : CounterAlias renames Counter;
        begin
            aliasInstance.Add;
        end Reenter;
        procedure Spawn is
            task NeverStarted;
            task body NeverStarted is
            begin
                null;
            end NeverStarted;
        begin
            null;
        end Spawn;
        function Get return Integer is
            function Nested return Integer is
            begin
                return Current;
            end Nested;
        begin
            return Nested;
        end Get;
        function Get (extra : Integer) return Integer is
        begin
            return Current + extra;
        end Get;
        function Snapshot return String is
        begin
            return text;
        end Snapshot;
    end Counter;
    first, second : Counter;
    aliasObject : Counter renames first;
    type Counters is array (1 .. 2) of Counter;
    arrayObjects : Counters;
    type Holder is limited record
        marker : Character := 'A';
        item : Counter;
    end record;
    holderObject : Holder;
    type Pointer is access Counter;
    procedure Free is new Ada.Unchecked_Deallocation (Counter, Pointer);
    pointerObject : Pointer := new Counter;
    nullObject : Pointer;
    references : Integer := 0;
    subtype Small is Integer range 0 .. 3;
    smallResult : Small := 1;
    function Reference return Pointer is
    begin
        references := references + 1;
        return pointerObject;
    end Reference;
    procedure Observe (object : Counter) is
    begin
        Check (object.Get >= 0);
    end Observe;
    package HiddenModel is
        type Hidden is limited private;
        procedure Update (object : in out Hidden);
    private
        protected type Hidden is
            procedure Increment;
            function Get return Integer;
        private
            value : Integer := 0;
        end Hidden;
    end HiddenModel;
    package body HiddenModel is
        protected body Hidden is
            procedure Increment is
            begin
                value := value + 1;
            end Increment;
            function Get return Integer is
            begin
                return value;
            end Get;
        end Hidden;
        procedure Update (object : in out Hidden) is
        begin
            object.Increment;
            Check (object.Get = 1);
        end Update;
    end HiddenModel;
    hiddenObject : HiddenModel.Hidden;
    protected Single is
        procedure Update;
        function Get return Integer;
    private
        value : Integer := 9;
    end Single;
    protected body Single is
        procedure Update is
        begin
            value := Single.value + 1;
            second.Add (2);
        end Update;
        function Get return Integer is
        begin
            return value;
        end Get;
    end Single;
begin
    Check (earlyResult = -1);
    earlyObject.Add;
    Check (earlyObject.Get = 2);
    aliasObject.Add (amount => 4);
    Check (first.Get = 5 and second.Get = 0);
    Check (first.Get (extra => 3) = 8);
    Observe (first);
    arrayObjects (1).Add;
    Check (arrayObjects (1).Get = 2 and arrayObjects (2).Get = 0);
    holderObject.item.Add (7);
    Check (holderObject.marker = 'A' and holderObject.item.Get = 8);
    Reference.Add (3);
    Check (references = 1 and pointerObject.Get = 4);
    Check (first.Snapshot = "abc");
    begin
        first.Fail;
        raise Program_Error;
    exception
        when occurrence : Constraint_Error =>
            Check (Ada.Exceptions.Exception_Message (occurrence) = "protected failure");
    end;
    first.Add;
    Check (first.Get = 8);
    first.Add (first.Get);
    Check (first.Get = 17);
    begin
        first.Reenter;
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    begin
        first.Spawn;
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    Check (first.Get = 17);
    begin
        first.SetResult (smallResult);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Check (smallResult = 1 and first.Get = 17);
    HiddenModel.Update (hiddenObject);
    Single.Update;
    Check (Single.Get = 10 and second.Get = 3);
    begin
        nullObject.Add;
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Free (pointerObject);
    Check (pointerObject = null);
    Ada.Text_IO.Put_Line ("protected objects passed");
end ProtectedObjects;

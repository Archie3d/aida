with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Ada.Finalization;
with System.Storage_Pools;
with System.Storage_Elements; use System.Storage_Elements;
with System;
with Storage_Pool_Globals;
procedure StoragePools is
    use type System.Address;
    use type Storage_Pool_Globals.Ref;
    procedure Check (Value : Boolean) is
    begin
        if not Value then raise Program_Error; end if;
    end Check;
    type Slot is record
        Address : System.Address;
        Size, Alignment : Storage_Count;
    end record;
    type Ledger is array (1 .. 128) of Slot;
    type Pool_Type is new System.Storage_Pools.Root_Storage_Pool with record
        Allocations, Releases : Natural := 0;
        Last_Size, Last_Alignment : Storage_Count := 0;
        Entries : Ledger := (others => (System.Null_Address, 0, 0));
        Refuse, Fail_Release : Boolean := False;
    end record;
    overriding procedure Allocate (Pool : in out Pool_Type;
        Storage_Address : out System.Address; Size_In_Storage_Elements, Alignment : Storage_Count);
    overriding procedure Deallocate (Pool : in out Pool_Type;
        Storage_Address : System.Address; Size_In_Storage_Elements, Alignment : Storage_Count);
    overriding function Storage_Size (Pool : Pool_Type) return Storage_Count;
    overriding procedure Finalize (Pool : in out Pool_Type);
    function Reserve (Size : Storage_Count) return System.Address;
    pragma Import (C, Reserve, "__ada_pool_default_allocate");
    procedure Release (Address : System.Address);
    pragma Import (C, Release, "__ada_pool_default_deallocate");
    overriding procedure Allocate (Pool : in out Pool_Type;
        Storage_Address : out System.Address; Size_In_Storage_Elements, Alignment : Storage_Count) is
    begin
        if Pool.Refuse then raise Storage_Error with "pool refused"; end if;
        Pool.Allocations := Pool.Allocations + 1;
        Pool.Last_Size := Size_In_Storage_Elements;
        Pool.Last_Alignment := Alignment;
        Storage_Address := Reserve (Size_In_Storage_Elements);
        for I in Pool.Entries'Range loop
            if Pool.Entries (I).Address = System.Null_Address then
                Pool.Entries (I) := (Storage_Address, Size_In_Storage_Elements, Alignment);
                return;
            end if;
        end loop;
        raise Program_Error;
    end Allocate;
    overriding procedure Deallocate (Pool : in out Pool_Type;
        Storage_Address : System.Address; Size_In_Storage_Elements, Alignment : Storage_Count) is
    begin
        Pool.Releases := Pool.Releases + 1;
        for I in Pool.Entries'Range loop
            if Pool.Entries (I).Address = Storage_Address then
                Check (Size_In_Storage_Elements = Pool.Entries (I).Size and Alignment = Pool.Entries (I).Alignment);
                Pool.Entries (I).Address := System.Null_Address;
                Release (Storage_Address);
                if Pool.Fail_Release then raise Constraint_Error with "pool release"; end if;
                return;
            end if;
        end loop;
        raise Program_Error;
    end Deallocate;
    overriding function Storage_Size (Pool : Pool_Type) return Storage_Count is
    begin
        return 4096;
    end Storage_Size;
    overriding procedure Finalize (Pool : in out Pool_Type) is
    begin
        Check (Pool.Allocations = Pool.Releases);
    end Finalize;
    Pool : Pool_Type;
    type Ref is access Integer;
    for Ref'Storage_Pool use Pool;
    procedure Free is new Ada.Unchecked_Deallocation (Integer, Ref);
    X : Ref := new Integer'(42);
    Global_Value : Storage_Pool_Globals.Ref := new Integer'(88);
    procedure Run is
        Local, Other : aliased Pool_Type;
        type Pool_Ref is access all System.Storage_Pools.Root_Storage_Pool'Class;
        Current : Pool_Ref := Local'Access;
        type Integers is access Integer;
        type Derived is new Integers;
        subtype View is Derived;
        -- The pool association can follow access derivation. Capture once.
        for Integers'Storage_Pool use Current.all;
        type Shared is access Integer;
        for Shared'Storage_Pool use Integers'Storage_Pool;
        procedure Drop is new Ada.Unchecked_Deallocation (Integer, Integers);
        procedure Drop_Derived is new Ada.Unchecked_Deallocation (Integer, Derived);
        procedure Drop_Shared is new Ada.Unchecked_Deallocation (Integer, Shared);
        A : Integers := new Integer'(1);
        B : Derived := new Integer'(2);
        C : Shared := new Integer'(3);
        Failed : Boolean := False;
        Calls : Natural := 0;
        function Bad_Value return Integer is
        begin
            Calls := Calls + 1;
            raise Constraint_Error with "initializer";
            return 0;
        end Bad_Value;
        type Text_Ptr is access String;
        for Text_Ptr'Storage_Pool use Local;
        procedure Drop_Text is new Ada.Unchecked_Deallocation (String, Text_Ptr);
        Text : Text_Ptr := new String'("hello");
        Empty_Text : Text_Ptr := new String (1 .. 0);
        type Base is tagged null record;
        type Child is new Base with record
            Number : Integer;
        end record;
        type Poly is access Base'Class;
        for Poly'Storage_Pool use Local;
        Source : Child := (Base with Number => 27);
        Wide : Poly := new Base'Class'(Source);
        procedure Drop_Poly is new Ada.Unchecked_Deallocation (Base'Class, Poly);
        type Byte is range 0 .. 255;
        for Byte'Size use 8;
        for Byte'Alignment use 0;
        type Byte_Ref is access Byte;
        for Byte_Ref'Storage_Pool use Local;
        Byte_Value : Byte_Ref := new Byte'(7);
        procedure Drop_Byte is new Ada.Unchecked_Deallocation (Byte, Byte_Ref);
        type Zero is access Integer;
        for Zero'Storage_Size use 0;
        type Zero_Derived is new Zero;
        type Default_Ref is access Integer;
        type Default_Shared is access Integer;
        for Default_Shared'Storage_Pool use Default_Ref'Storage_Pool;
        Default_Value : Default_Shared := new Integer'(77);
        procedure Drop_Default is new Ada.Unchecked_Deallocation (Integer, Default_Shared);
        Finalized : Natural := 0;
        Fail_Init : Boolean := False;
        type Guard is new Ada.Finalization.Limited_Controlled with record
            Value : Integer := 19;
        end record;
        overriding procedure Initialize (Object : in out Guard);
        overriding procedure Finalize (Object : in out Guard);
        overriding procedure Initialize (Object : in out Guard) is
        begin
            if Fail_Init then raise Constraint_Error; end if;
        end Initialize;
        overriding procedure Finalize (Object : in out Guard) is
        begin
            Check (Object.Value = 19);
            Finalized := Finalized + 1;
        end Finalize;
    begin
        Check (Local.Last_Size = 1 and Local.Last_Alignment = 0 and Byte_Value.all = 7);
        Current := Other'Access;
        A.all := 7;
        declare
            Extra : Integers := new Integer'(9);
        begin
            Check (Local.Allocations = 8 and Other.Allocations = 0);
            Drop (Extra);
        end;
        Check (View'Storage_Pool'Address = Local'Address and Derived'Storage_Size = 4096);
        Check (A.all = 7 and B.all = 2 and C.all = 3 and Text.all = "hello");
        Check (Empty_Text.all'Length = 0 and Child (Wide.all).Number = 27);
        Check (Zero'Storage_Size = 0 and Zero_Derived'Storage_Size = 0);
        Check (Default_Ref'Storage_Size > 0 and Default_Value.all = 77);
        Drop_Default (Default_Value);
        Drop (A);
        Drop_Derived (B);
        Drop_Shared (C);
        Drop_Text (Text);
        Drop_Text (Empty_Text);
        Drop_Poly (Wide);
        Drop_Byte (Byte_Value);
        Check (A = null and B = null and C = null and Text = null);
        Local.Refuse := True;
        begin
            A := new Integer'(Bad_Value);
        exception
            when Storage_Error => Failed := True;
        end;
        Check (Failed and A = null and Calls = 0);
        Local.Refuse := False;
        Failed := False;
        begin
            A := new Integer'(Bad_Value);
        exception
            when Constraint_Error => Failed := True;
        end;
        Check (Failed and Calls = 1 and A = null and Local.Allocations = Local.Releases);
        declare
            type Guards is access Guard;
            for Guards'Storage_Pool use Local;
            procedure Drop_Guard is new Ada.Unchecked_Deallocation (Guard, Guards);
            Good : Guards := new Guard;
            Bad : Guards;
        begin
            Check (Good.Value = 19 and Local.Last_Alignment = Guard'Alignment
                and Local.Last_Size <= Guard'Max_Size_In_Storage_Elements);
            Drop_Guard (Good);
            Check (Finalized = 1);
            Fail_Init := True;
            Failed := False;
            begin
                Bad := new Guard;
            exception
                when Constraint_Error => Failed := True;
            end;
            Check (Failed and Bad = null and Finalized = 1);
            Fail_Init := False;
            Good := new Guard;
            -- Collection cleanup must finalize this object before Pool.Finalize.
        end;
        Check (Finalized = 2 and Local.Allocations = Local.Releases);
        declare
            protected Completion is
                procedure Done;
                function Count return Natural;
            private
                Value : Natural := 0;
            end Completion;
            protected body Completion is
                procedure Done is begin Value := Value + 1; end Done;
                function Count return Natural is begin return Value; end Count;
            end Completion;
            task type Job;
            task body Job is
            begin
                Completion.Done;
            end Job;
            type Jobs is access Job;
            for Jobs'Storage_Pool use Local;
            procedure Drop_Job is new Ada.Unchecked_Deallocation (Job, Jobs);
            Work : Jobs := new Job;
        begin
            Drop_Job (Work);
            Check (Work = null and Completion.Count = 1 and Local.Allocations = Local.Releases);
        end;
        Local.Fail_Release := True;
        A := new Integer'(5);
        Failed := False;
        begin
            Drop (A);
        exception
            when Constraint_Error => Failed := True;
        end;
        A := null;
        Check (Failed and Local.Allocations = Local.Releases);
    end Run;
    protected Results is
        procedure Done;
        function Count return Natural;
    private
        Value : Natural := 0;
    end Results;
    protected body Results is
        procedure Done is begin Value := Value + 1; end Done;
        function Count return Natural is begin return Value; end Count;
    end Results;
    task type Worker;
    task body Worker is
    begin
        Run;
        Results.Done;
    end Worker;
begin
    Check (Global_Value.all = 88 and Storage_Pool_Globals.Pool.Outstanding = 1);
    Storage_Pool_Globals.Drop (Global_Value);
    Check (Global_Value = null and Storage_Pool_Globals.Pool.Outstanding = 0);
    Check (X.all = 42 and Pool.Allocations = 1 and Pool.Last_Size = 4 and Pool.Last_Alignment = 4);
    Check (Ref'Storage_Size = 4096 and Ref'Storage_Pool'Address = Pool'Address);
    Free (X);
    Check (X = null and Pool.Releases = 1);
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Check (Results.Count = 4);
    Put_Line ("storage pools ok");
end StoragePools;

with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure ConcurrentResults is
    procedure Run is
        protected Count is
            procedure Add;
            function Get return Integer;
        private
            Value : Integer := 0;
        end Count;
        protected body Count is
            procedure Add is begin Value := Value + 1; end Add;
            function Get return Integer is begin return Value; end Get;
        end Count;
        function Start return Integer is
        begin
            Count.Add;
            return 0;
        end Start;
        task type Worker;
        task body Worker is
            Activated : Integer := Start;
        begin null; end Worker;
        function Make return Worker is
        begin
            return Result : Worker;
        end Make;
        function Forward return Worker is begin return Make; end Forward;
        protected type Guard is
            procedure Set (N : Integer);
            function Get return Integer;
        private
            Value : Integer := 0;
        end Guard;
        protected body Guard is
            procedure Set (N : Integer) is begin Value := N; end Set;
            function Get return Integer is begin return Value; end Get;
        end Guard;
        function MakeGuard return Guard is
        begin
            return Result : Guard do Result.Set (17); end return;
        end MakeGuard;
        type GuardFactory is access function return Guard;
        GuardConstructor : GuardFactory := MakeGuard'Access;
        type Bundle is record
            W : Worker;
            G : Guard;
            N : Integer;
        end record;
        function MakeBundle return Bundle is
        begin
            return (W => Forward, G => GuardConstructor.all, N => 42);
        end MakeBundle;
        type Workers is array (Positive range <>) of Worker;
        function MakeWorkers return Workers is
        begin
            return (1 .. 3 => Make);
        end MakeWorkers;
        function ExtendedWorkers return Workers is
        begin
            return Result : Workers := MakeWorkers;
        end ExtendedWorkers;
        type Defaulted is record
            W : Worker := Make;
        end record;
        function Failed return Bundle is
        begin
            return Result : Bundle := MakeBundle do
                raise Constraint_Error;
            end return;
        exception
            when Constraint_Error =>
                return (W => <>, G => MakeGuard, N => 9);
        end Failed;
        function Abandon return Worker is
        begin
            return Result : Worker do goto Retry; end return;
            <<Retry>>
            return Make;
        end Abandon;
        type Factory is access function return Worker;
        Constructor : Factory := Make'Access;
        type LocalGuard is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Finalize (Object : in out LocalGuard);
        overriding procedure Finalize (Object : in out LocalGuard) is
        begin
            raise Constraint_Error;
        end Finalize;
        function FailedFinalizer return Worker is
            Local : LocalGuard;
        begin
            return Make;
        end FailedFinalizer;
        procedure Observe (Item : Bundle; Expected : Integer) is
        begin
            if Item.G.Get /= 17 or Count.Get /= Expected then
                raise Program_Error;
            end if;
        end Observe;
        generic
            type Item is limited private;
            with function Construct return Item;
        package Holder is
            Object : Item := Construct;
        end Holder;
        generic
            Object : Bundle;
        package BundleHolder is end BundleHolder;
        generic
            Objects : Workers;
        package WorkersHolder is end WorkersHolder;
        type BundleAccess is access Bundle;
        type WorkersAccess is access Workers;
    begin
        begin
            declare
                Discarded : Worker := FailedFinalizer;
            begin raise Program_Error; end;
        exception
            when Constraint_Error | Program_Error => null;
        end;
        if Count.Get /= 0 then raise Program_Error; end if;
        declare
            A : Worker := Constructor.all;
            B : Bundle := MakeBundle;
            C : Workers := ExtendedWorkers;
            D : Bundle := Failed;
            E : Worker := Abandon;
            F : Bundle := (W => <>, G => <>, N => 1);
            G : Defaulted := (W => <>);
            -- The entire declarative part is elaborated before any of these tasks runs.
            Snapshot : Integer := Count.Get;
        begin
            if Snapshot /= 0 or B.G.Get /= 17 or D.G.Get /= 17
                or B.N /= 42 or D.N /= 9 or F.G.Get /= 0
            then raise Program_Error; end if;
        end;
        if Count.Get /= 9 then raise Program_Error; end if;
        Observe (MakeBundle, 10);
        Observe ((W => <>, G => MakeGuard, N => 1), 11);
        declare
            package TaskHolder is new Holder (Worker, Make);
            package GuardHolder is new Holder (Guard, MakeGuard);
        begin
            if Count.Get /= 12 or GuardHolder.Object.Get /= 17 then
                raise Program_Error;
            end if;
        end;
        declare
            P : BundleAccess := new Bundle'(MakeBundle);
            Q : WorkersAccess := new Workers'(MakeWorkers);
        begin
            if P.G.Get /= 17 then raise Program_Error; end if;
        end;
        declare
            package One is new BundleHolder (MakeBundle);
            package Several is new WorkersHolder ((1 .. 2 => <>));
        begin null; end;
        if Count.Get /= 19 then raise Program_Error; end if;
        -- Access collections live until this procedure's master finishes.
    end Run;
    task type Client;
    task body Client is begin Run; end Client;
begin
    Run;
    declare
        Clients : array (1 .. 4) of Client;
    begin null; end;
    Put_Line ("concurrent results OK");
end ConcurrentResults;

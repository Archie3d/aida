with Ada.Text_IO; use Ada.Text_IO;
procedure InterfaceSelect is
    package Model is
        type I is limited interface;
        procedure Run (Self : in out I; Value : out Integer) is abstract;
        type S is synchronized interface and I;
        protected type P (N : Integer := 5) is new S with
            overriding entry Run (Value : out Integer);
            procedure Open;
            function Called return Boolean;
        private
            Ready : Boolean := False;
            Done : Boolean := False;
        end P;
        protected Gate is
            entry Wait;
            procedure Open;
            procedure Finish;
            function Done return Boolean;
        private
            Ready : Boolean := False;
            Completed : Boolean := False;
        end Gate;
        task type T is new S with
            overriding entry Run (Value : out Integer);
        end T;
        type R is limited new I with null record;
        overriding procedure Run (Self : in out R; Value : out Integer);
    end Model;
    package body Model is
        protected body P is
            entry Run (Value : out Integer) when Ready is
            begin Value := N; Done := True; end Run;
            procedure Open is begin Ready := True; end Open;
            function Called return Boolean is begin return Done; end Called;
        end P;
        protected body Gate is
            entry Wait when Ready is begin null; end Wait;
            procedure Open is begin Ready := True; end Open;
            procedure Finish is begin Completed := True; end Finish;
            function Done return Boolean is begin return Completed; end Done;
        end Gate;
        task body T is
        begin
            Gate.Wait;
            accept Run (Value : out Integer) do
                Value := 31;
                Gate.Finish;
            end Run;
        end T;
        overriding procedure Run (Self : in out R; Value : out Integer) is
        begin Value := 99; end Run;
    end Model;
    use Model;
    X : P;
    Y : R;
    Task_Object : T;
    V : Integer := -1;
    procedure Conditional (Z : in out I'Class; Expected : Boolean) is
        Selected : Boolean := False;
    begin
        select Run (Z, V); Selected := True;
        else null;
        end select;
        if Selected /= Expected then raise Program_Error; end if;
    end Conditional;
    procedure Timed (Z : in out I'Class) is
    begin
        select Run (Z, V); raise Program_Error;
        or delay 0.0;
        end select;
    end Timed;
    procedure Async (Z : in out I'Class; Blocking : Boolean) is
        Entered : Boolean := False;
    begin
        select
            Run (Z, V);
        then abort
            Entered := True;
            X.Open;
            while not X.Called loop delay 0.0; end loop;
        end select;
        if Entered /= Blocking then raise Program_Error; end if;
    end Async;
    procedure Async_Task (Z : in out I'Class) is
    begin
        select
            Run (Z, V);
        then abort
            Gate.Open;
            while not Gate.Done loop delay 0.0; end loop;
        end select;
    end Async_Task;
begin
    Conditional (X, False);
    if V /= -1 then raise Program_Error; end if;
    Timed (X);
    Async (X, True);
    if V /= 5 then raise Program_Error; end if;
    Conditional (Y, True);
    if V /= 99 then raise Program_Error; end if;
    Async (Y, False);
    Async_Task (Task_Object);
    if V /= 31 then raise Program_Error; end if;
    Put_Line ("interface select OK");
end InterfaceSelect;

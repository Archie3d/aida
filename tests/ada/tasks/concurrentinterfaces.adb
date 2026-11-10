with Ada.Text_IO; use Ada.Text_IO;
with Ada.Tags;
with Ada.Unchecked_Deallocation;
procedure ConcurrentInterfaces is
    use type Ada.Tags.Tag;
    Captured_Limit : Integer := 5;
    package Model is
        type Base is limited interface;
        procedure Run (Self : in out Base; Value : out Integer) is abstract;
        procedure Nothing (Self : in out Base) is null;
        type Left is synchronized interface and Base;
        type Right is synchronized interface and Base;
        type Contract is synchronized interface and Left and Right;
        type Task_Contract is task interface and Contract;
        type Protected_Contract is protected interface and Contract;
        type Access_Contract is protected interface;
        procedure Access_Run (Self : access Access_Contract; Value : out Integer) is abstract;
        type Ref is access all Base'Class;
        type Access_Ref is access all Access_Contract'Class;
        Registered : Ref;
        task type T (N : Integer := 17) is new Task_Contract and Task_Contract with
            overriding entry Run (Value : out Integer);
        end T;
        protected type P (N : Integer := 3) is new Protected_Contract and Protected_Contract and Access_Contract with
            overriding procedure Run (Value : out Integer);
            procedure Current (Value : out Ref);
            overriding procedure Access_Run (Value : out Integer);
        private
            Data : String (1 .. N) := (others => 'x');
            Captured : String (1 .. Captured_Limit) := (others => 'c');
        end P;
    end Model;
    package body Model is
        task body T is
        begin
            Registered := T'Unchecked_Access;
            accept Run (Value : out Integer) do Value := N; end Run;
        end T;
        protected body P is
            procedure Run (Value : out Integer) is
            begin
                if Captured /= "ccccc" then raise Program_Error; end if;
                Value := Data'Length;
            end Run;
            procedure Current (Value : out Ref) is begin Value := P'Unchecked_Access; end Current;
            procedure Access_Run (Value : out Integer) is begin Value := Data'Length; end Access_Run;
        end P;
    end Model;
    use Model;
    X : aliased T;
    Y : aliased P;
    A : Ref := X'Access;
    B : Ref := Y'Access;
    Access_View : Access_Ref := Y'Access;
    Value : Integer;
    Current : Ref;
    procedure Free is new Ada.Unchecked_Deallocation (Base'Class, Ref);
    procedure Call (Self : in out Base'Class; Expected : Integer) is
    begin
        Run (Self, Value);
        if Value /= Expected then raise Program_Error; end if;
        Nothing (Self);
    end Call;
    type Callback is access procedure (Self : in out P; Value : out Integer);
    Invoke : Callback := Run'Access;
begin
    Call (A.all, 17);
    Call (B.all, 3);
    Access_Run (Access_View, Value);
    if Value /= 3 then raise Program_Error; end if;
    Y.Current (Current);
    if Registered /= A or else Current /= B then raise Program_Error; end if;
    if Ada.Tags.Interface_Ancestor_Tags (T'Tag)'Length /= 5 then raise Program_Error; end if;
    if not (A.all in Task_Contract'Class) or else (A.all in Protected_Contract'Class)
        or else not (B.all in Contract'Class) then raise Program_Error; end if;
    Run (P (B.all), Value);
    if Value /= 3 then raise Program_Error; end if;
    Invoke (Y, Value);
    if Value /= 3 then raise Program_Error; end if;
    begin
        Run (P (A.all), Value);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    A := new T (23);
    Call (A.all, 23);
    Free (A);
    B := new P (7);
    Call (B.all, 7);
    Free (B);
    if A /= null or else B /= null then raise Program_Error; end if;
    Put_Line ("concurrent interfaces OK");
end ConcurrentInterfaces;

with Ada.Text_IO; use Ada.Text_IO;
procedure CallbackDereference is
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    procedure Exercise is
        Total : Integer := 0;
        Evaluations : Integer := 0;
        type Action is access procedure (Amount : Integer);
        type Query is access function return Integer;
        type Other_Action is access procedure (Amount : Integer);
        type Derived_Action is new Action;
        procedure Add (Amount : Integer) is
        begin
            Total := Total + Amount;
        end Add;
        function Read return Integer is
        begin
            return Total;
        end Read;
        A : Action := Add'Access;
        Q : Query := Read'Access;
        B : Other_Action := A.all'Access;
        D : Derived_Action := A.all'Access;
        type Holder is record
            Callback : Action;
        end record;
        Object : Holder := (Callback => A);
        type Actions is array (1 .. 2) of Action;
        Items : Actions := (A, A);
        function Choose return Action is
        begin
            Evaluations := Evaluations + 1;
            return A;
        end Choose;
        function Choose return Query is
        begin
            raise Program_Error;
            return Q;
        end Choose;
        function Empty_Action return Action is
        begin
            Evaluations := Evaluations + 1;
            return null;
        end Empty_Action;
        function Select_Action (Index : Integer) return Action is
        begin
            Evaluations := Evaluations + Index;
            return A;
        end Select_Action;
        procedure Invoke (Callback : access procedure (Amount : Integer)) is
        begin
            Callback (3);
        end Invoke;
        procedure Forward (Callback : access procedure (Amount : Integer)) is
        begin
            Invoke (Callback.all'Access);
        end Forward;
        protected Counter is
            procedure Increment (Amount : Integer);
            function Value return Integer;
        private
            Count : Integer := 0;
        end Counter;
        protected body Counter is
            procedure Increment (Amount : Integer) is
            begin
                Count := Count + Amount;
            end Increment;
            function Value return Integer is
            begin
                return Count;
            end Value;
        end Counter;
        Stored : access protected procedure (Amount : Integer) := Counter.Increment'Access;
        procedure Protected_Invoke (Callback : access protected procedure (Amount : Integer)) is
        begin
            Callback (4);
        end Protected_Invoke;
        procedure Protected_Forward (Callback : access protected procedure (Amount : Integer)) is
        begin
            Protected_Invoke (Callback.all'Access);
        end Protected_Forward;
        function Absolute (Value : Integer) return Integer;
        pragma Import (C, Absolute, "abs");
        type C_Query is access function (Value : Integer) return Integer;
        pragma Convention (C, C_Query);
        Native : C_Query := Absolute'Access;
        Native_Copy : C_Query := Native.all'Access;
        Empty : Action;
        Empty_Query : Query;
    begin
        -- Taking Access does not invoke either designated procedure/function.
        Check (Total = 0 and B = A.all'Access);
        B (1);
        D (2);
        B := Choose.all'Access;
        Check (Evaluations = 1 and Total = 3);
        B (4);
        B := Select_Action (2).all'Access;
        Check (Evaluations = 3 and Total = 7);
        B (5);
        B := Object.Callback.all'Access;
        B (6);
        B := Items (2).all'Access;
        B (7);
        Q := Q.all'Access;
        Check (Total = 25 and Q.all = 25);
        Forward (A.all'Access);
        Invoke (Choose.all'Access);
        Check (Evaluations = 4 and Total = 31);
        Protected_Forward (Stored.all'Access);
        Check (Counter.Value = 4 and Native_Copy (-9) = 9);
        declare
            type Inner_Action is access procedure (Amount : Integer);
            Inner : Inner_Action := A.all'Access;
        begin
            Inner (1);
        end;
        Check (Total = 32);
        begin
            B := Empty.all'Access;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            B := Empty_Action.all'Access;
            raise Program_Error;
        exception
            when Constraint_Error => Check (Evaluations = 5);
        end;
        begin
            Q := Empty_Query.all'Access;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        -- Failed dereferences leave the previous callback values intact.
        B (1);
        Check (Total = 33 and Q.all = 33);
    end Exercise;
    protected Results is
        procedure Done;
        function Count return Integer;
    private
        Total : Integer := 0;
    end Results;
    protected body Results is
        procedure Done is
        begin
            Total := Total + 1;
        end Done;
        function Count return Integer is
        begin
            return Total;
        end Count;
    end Results;
begin
    Exercise;
    declare
        task type Worker;
        task body Worker is
        begin
            Exercise;
            Results.Done;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Check (Results.Count = 4);
    Put_Line ("callback dereference passed");
end CallbackDereference;

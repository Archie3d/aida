with Ada.Text_IO;
procedure Callbackprofiles is
    Total : Integer := 0;
    type Link is access all Integer;
    type Action is access procedure;
    type Input is access procedure (Item : in Integer);
    type Output is access procedure (Item : out Integer);
    type Update is access procedure (Item : in out Integer);
    type Anonymous is access procedure (Item : access Integer);
    type Named is access procedure (Item : Link);
    type Query is access function return Integer;
    type Map is access function (Item : Integer) return Integer;
    type Protected_Action is access protected procedure;
    type Protected_Query is access protected function return Integer;
    procedure Bump is
    begin
        Total := Total + 1;
    end Bump;
    procedure Add (Value : Integer) is
    begin
        Total := Total + Value;
    end Add;
    procedure Defaulted_Add (Value : in Integer := 10) is
    begin
        Total := Total + Value;
    end Defaulted_Add;
    procedure Set (Value : out Integer) is
    begin
        Value := 7;
    end Set;
    procedure Increment (Value : in out Integer) is
    begin
        Value := Value + 3;
    end Increment;
    procedure Follow (Value : access Integer) is
    begin
        Value.all := Value.all + 4;
    end Follow;
    procedure Follow_Named (Value : Link) is
    begin
        Value.all := Value.all + 5;
    end Follow_Named;
    function Forty_Two return Integer is
    begin
        return 42;
    end Forty_Two;
    function Double (Value : Integer) return Integer is
    begin
        return Value * 2;
    end Double;
    protected Counter is
        procedure Add;
        function Value return Integer;
    private
        Count : Integer := 0;
    end Counter;
    protected body Counter is
        procedure Add is
        begin
            Count := Count + 1;
        end Add;
        function Value return Integer is
        begin
            return Count;
        end Value;
    end Counter;
    -- A local wrapper has an ordinary profile, although it calls a formal.
    generic
        with procedure Operation (Value : in out Integer);
    procedure Apply (Value : in out Integer);
    procedure Apply (Value : in out Integer) is
        procedure Forward (Value : in out Integer) is
        begin
            Operation (Value);
        end Forward;
        type Local_Update is access procedure (Value : in out Integer);
        Callback : Local_Update := Forward'Access;
    begin
        Callback (Value);
    end Apply;
    procedure Apply_Increment is new Apply (Increment);
    -- A formal object whose value is a callback is also legal.
    generic
        Callback : Update;
    procedure Invoke (Value : in out Integer);
    procedure Invoke (Value : in out Integer) is
    begin
        Callback (Value);
    end Invoke;
    A : Action := Bump'Access;
    I : Input := Add'Access;
    D : Input := Defaulted_Add'Access;
    O : Output := Set'Access;
    U : Update := Increment'Access;
    P : Anonymous := Follow'Access;
    N : Named := Follow_Named'Access;
    F : Query := Forty_Two'Access;
    M : Map := Double'Access;
    PA : Protected_Action := Counter.Add'Access;
    PQ : Protected_Query := Counter.Value'Access;
    procedure Invoke_Increment is new Invoke (U);
    Number : aliased Integer := 0;
begin
    A.all;
    I (2);
    D (3);
    O (Number);
    U (Number);
    P (Number'Access);
    N (Number'Access);
    Apply_Increment (Number);
    Invoke_Increment (Number);
    PA.all;
    if Total /= 6 or Number /= 25 or F.all /= 42 or M (4) /= 8 or PQ.all /= 1 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("callback profiles passed");
end Callbackprofiles;

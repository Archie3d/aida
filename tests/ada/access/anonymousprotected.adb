with Ada.Text_IO; use Ada.Text_IO;
procedure AnonymousProtected is
    protected Counter is
        procedure Add (Amount : Integer);
        function Read return Integer;
    private
        Value : Integer := 1;
    end Counter;
    protected body Counter is
        procedure Add (Amount : Integer) is
        begin
            Value := Value + Amount;
        end Add;
        function Read return Integer is
        begin
            return Value;
        end Read;
    end Counter;
    procedure Invoke (Action : access protected procedure (Amount : Integer)) is
    begin
        Action (3);
    end Invoke;
    function Observe (Action : access protected function return Integer) return Integer is
    begin
        return Action.all;
    end Observe;
    Stored : access protected procedure (Amount : Integer) := Counter.Add'Access;
begin
    Invoke (Counter.Add'Access);
    Stored (2);
    if Observe (Counter.Read'Access) /= 6 then raise Program_Error; end if;
    Put_Line ("anonymous protected callbacks passed");
end AnonymousProtected;

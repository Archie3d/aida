package body Concurrent_Interface_Model is
    task body Worker is
    begin
        loop
            select
                accept Add (Value : in out Integer) do
                    Value := Value + Seed;
                end Add;
            or
                terminate;
            end select;
        end loop;
    end Worker;
    protected body Counter is
        procedure Add (Value : in out Integer) is
        begin
            Value := Value + Data'Length;
        end Add;
    end Counter;
    procedure Apply (Self : in out Actual; Value : in out Integer) is
    begin
        Add (Self, Value);
    end Apply;
end Concurrent_Interface_Model;

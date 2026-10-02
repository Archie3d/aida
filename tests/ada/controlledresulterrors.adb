with Controlled_Result_Model; use Controlled_Result_Model;
procedure ControlledResultErrors is
    function Foreign return Guard;
    pragma Import (C, Foreign, "foreign_guard");
    type Class_Factory is access function return Guard'Class;
begin
    null;
end ControlledResultErrors;

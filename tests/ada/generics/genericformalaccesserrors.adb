procedure Genericformalaccesserrors is
    type Action_Access is access procedure;
    type Query_Access is access function return Integer;
    generic
        with procedure Action;
    package Template is
    end Template;
    package body Template is
        Named : Action_Access := Action'Access;
        Anonymous : access procedure := Action'Access;
    end Template;
    generic
        with function Query return Integer;
    function Read return Integer;
    function Read return Integer is
        Callback : Query_Access := Query'Access;
    begin
        return 0;
    end Read;
    generic
        with procedure Action is null;
    procedure Defaulted;
    procedure Defaulted is
        Callback : Action_Access := Action'Access;
    begin
        null;
    end Defaulted;
begin
    null;
end Genericformalaccesserrors;

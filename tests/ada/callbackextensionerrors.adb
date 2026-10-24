procedure CallbackExtensionErrors is
    function Foreign (Value : Integer) return Integer;
    pragma Import (C, Foreign, "abs");
    function Local (Value : Integer) return Integer is
    begin
        return Value;
    end Local;
    type Ada_Function is access function (Value : Integer) return Integer;
    type C_Function is access function (Value : Integer) return Integer;
    pragma Convention (C, C_Function);
    A : Ada_Function := Foreign'Access;
    C : C_Function := Local'Access;
    type Bad_Profile is access function return String;
    pragma Convention (C, Bad_Profile);
    type Late_Type is access function return Integer;
    function Prior_Use return Late_Type;
    pragma Convention (C, Late_Type);
    function Prior_Use return Late_Type is
    begin
        return null;
    end Prior_Use;
    function Late_Import (Value : Integer) return Integer;
    B : Ada_Function := Late_Import'Access;
    pragma Import (Ada, Late_Import, "external_function");
    pragma Convention (C, Local);
begin
    null;
end CallbackExtensionErrors;

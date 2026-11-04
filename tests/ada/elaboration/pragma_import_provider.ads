package Pragma_Import_Provider is
    procedure Fail (Text : String);
    procedure Foreign_Fail (Text : String);
    pragma Import (Ada, Foreign_Fail, "pragma_import_provider__fail");
    pragma No_Return (Foreign_Fail);
    type Callback is access procedure (Text : String);
    function Get return Callback;
end Pragma_Import_Provider;

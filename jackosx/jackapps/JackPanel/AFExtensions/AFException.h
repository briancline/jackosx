#define ThrowAFException(routine, err) if (err != 0) \
            [[NSException exceptionWithName:@"AFException" reason:routine userInfo:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:err] forKey:@"Error"]] raise]

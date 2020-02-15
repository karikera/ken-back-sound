module.exports = {
    files: [
        ['ken-res-loader/include', 'include/ken-back', ['**/*.h']],
        ['ken-res-loader/jpeg/README', 'jpeg.README'],
		['ken-res-loader/libogg/README.md', 'libogg.README'],
		['ken-res-loader/libvorbis/README.md', 'libvorbis.README.md'],
		['ken-res-loader/zlib/README', 'zlib.README'],
    ],
	each(krb)
	{
		krb.vsbuild('ken-res-loader.sln');
        krb.copylib();

        const platshort = krb.platform.shortName;
        krb.copylib('libvorbis', `ken-res-loader/libvorbis/lib/${platshort}`);
        krb.copylib('libvorbisfile', `ken-res-loader/libvorbis/lib/${platshort}`);
        krb.copylib('jpeg', `ken-res-loader/jpeg/lib/${platshort}`);
        krb.copylib('libpng16', `ken-res-loader/libpng/lib/${platshort}`);
        krb.copylib('zlib', `ken-res-loader/zlib/${platshort}/lib`);
        if (krb.platform.name === 'js')
        {
            krb.copylib('libogg_static', `ken-res-loader/libogg/lib/${platshort}`);
        }
	}
};

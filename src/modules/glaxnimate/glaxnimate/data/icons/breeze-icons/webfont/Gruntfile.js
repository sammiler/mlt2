module.exports = function(grunt) {
    'use strict';

    grunt.initConfig({
        // Create the webfont files:
        webfont: {
                icons: {
                        src: '../icons/actions/22@2x/*.svg',
                        dest: './dist',
                        options: {
                            skipLinks: true,
                            engine: 'fontforge',
                            stylesheet: 'css',
                            types:  'eot,woff2,ttf,svg',
                            optimize: false
                        }
                }
        }
    });
    grunt.loadNpmTasks('grunt-webfonts');

    // Run the default task by executing "grunt" in the CLI:
    grunt.registerTask('default', ['webfont']);
};


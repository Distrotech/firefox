# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
from setuptools import setup

PACKAGE_VERSION = '0.16'

# we only support python 2 right now
assert sys.version_info[0] == 2

deps = ["ManifestDestiny >= 0.5.4",
        "mozfile >= 0.12"]

setup(name='mozprofile',
      version=PACKAGE_VERSION,
      description="Library to create and modify Mozilla application profiles",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=['Environment :: Console',
                   'Intended Audience :: Developers',
                   'License :: OSI Approved :: Mozilla Public License 2.0 (MPL 2.0)',
                   'Natural Language :: English',
                   'Operating System :: OS Independent',
                   'Programming Language :: Python',
                   'Topic :: Software Development :: Libraries :: Python Modules',
                   ],
      keywords='mozilla',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL 2.0',
      packages=['mozprofile'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      tests_require=['mozhttpd', 'mozfile'],
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      mozprofile = mozprofile:cli
      view-profile = mozprofile:view_profile
      diff-profiles = mozprofile:diff_profiles
      """,
    )
